// observable_state.hpp
#pragma once

#include <QObject>
#include <QPointer>

#include <functional>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

#include "devtools/state_registry.hpp"

namespace rui {

namespace detail {

// 读取追踪器：derivedStateOf 求值期间，MutableState::get() 会把读取来源上报给它，
// 并在首次遇到某个来源时注册观察者，依赖变化时触发重算。
class ReadTracker {
public:
    static ReadTracker* current() { return current_; }
    static void begin(ReadTracker* t) { current_ = t; }
    static void end() { current_ = nullptr; }

    void setOnInvalidate(std::function<void()> cb) { onInvalidate_ = std::move(cb); }
    std::function<void()> invalidationCallback() const { return onInvalidate_; }

    // 上报一次读取；同一来源只会调用一次 registerObserver。
    void onRead(const void* source, std::function<void()> registerObserver) {
        if (seen_.insert(source).second) {
            if (registerObserver) {
                registerObserver();
            }
        }
    }

private:
    static thread_local ReadTracker* current_;
    std::unordered_set<const void*> seen_;
    std::function<void()> onInvalidate_;
};

inline thread_local ReadTracker* ReadTracker::current_ = nullptr;

} // namespace detail

template <typename T>
class MutableState {
public:
    using Observer = std::function<void(const T&)>;

    MutableState(T initial_value) : value_(std::move(initial_value)) {}

    T get() const {
        if (auto* tracker = detail::ReadTracker::current()) {
            auto cb = tracker->invalidationCallback();
            tracker->onRead(this, [this, cb]() {
                const_cast<MutableState*>(this)->observe([cb](const T&) {
                    if (cb) {
                        cb();
                    }
                });
            });
        }
        std::lock_guard lock(mutex_);
        return value_;
    }

    void set(T new_value) {
        bool changed = false;
        std::string strVal;
        std::vector<ObserverEntry> observers;
        T newValue;
        {
            std::lock_guard lock(mutex_);
            if (value_ != new_value) {
                value_ = new_value;
                newValue = value_;
                strVal = valueToStringLocked();
                changed = true;
                observers = observers_;
            }
        }
        if (changed) {
            for (const auto& entry : observers) {
                if (entry.hasOwner && entry.owner.isNull()) {
                    continue;
                }
                entry.fn(newValue);
            }
        }
        if (changed && onChange_) {
            onChange_(strVal);
        }
    }

    void observe(Observer observer) {
        std::lock_guard lock(mutex_);
        ObserverEntry entry;
        entry.fn = std::move(observer);
        observers_.push_back(std::move(entry));
    }

    // 绑定 owner 的观察者：owner 销毁后自动跳过通知，避免悬垂指针。
    void observe(QObject* owner, Observer observer) {
        std::lock_guard lock(mutex_);
        ObserverEntry entry;
        entry.owner = QPointer<QObject>(owner);
        entry.hasOwner = true;
        entry.fn = std::move(observer);
        observers_.push_back(std::move(entry));
    }

    // 内部：设置 DevTools 变更回调（接收已计算好的字符串值）
    void setOnChange(std::function<void(const std::string&)> cb) { onChange_ = std::move(cb); }

    // 调用者必须已持有锁
    std::string valueToStringLocked() const {
        std::ostringstream oss;
        oss << value_;
        return oss.str();
    }

    // 线程安全版本
    std::string valueToString() const {
        std::lock_guard lock(mutex_);
        std::ostringstream oss;
        oss << value_;
        return oss.str();
    }

private:
    struct ObserverEntry {
        QPointer<QObject> owner;
        bool hasOwner = false;
        Observer fn;
    };

    mutable std::mutex mutex_;
    T value_;
    std::vector<ObserverEntry> observers_;
    std::function<void(const std::string&)> onChange_;
};

// 无 tag，不注册 DevTools
template <typename T>
auto mutableStateOf(T initial_value) {
    return std::make_shared<MutableState<T>>(std::move(initial_value));
}

// 带 tag，自动注册到 StateRegistryMgr（DevTools 可见）
template <typename T>
auto mutableStateOf(const std::string& tag, T initial_value) {
    auto state = std::make_shared<MutableState<T>>(std::move(initial_value));

    // 注册到 StateRegistryMgr
    StateRegistryMgr::Instance()->registerState(tag, state,
        [state] {
            return state->valueToString();
        }
    );

    // 变更时通知 StateRegistryMgr（strVal 已在 set() 内计算好，无需再加锁）
    state->setOnChange([tag](const std::string& strVal) {
        StateRegistryMgr::Instance()->notifyChange(tag, strVal);
    });

    return state;
}

} // namespace rui
