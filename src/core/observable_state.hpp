// observable_state.hpp
#pragma once

#include <QObject>
#include <QPointer>
#include <algorithm>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

#include "devtools/state_registry.hpp"

namespace rui {

class ObserverHandle {
public:
    ObserverHandle() = default;
    explicit ObserverHandle(std::function<void()> cancel)
        : cancel_(std::move(cancel))
    {
    }
    ObserverHandle(const ObserverHandle&) = delete;
    ObserverHandle& operator=(const ObserverHandle&) = delete;
    ObserverHandle(ObserverHandle&& other) noexcept
        : cancel_(std::move(other.cancel_))
    {
    }
    ObserverHandle& operator=(ObserverHandle&& other) noexcept
    {
        if (this != &other) {
            cancel();
            cancel_ = std::move(other.cancel_);
        }
        return *this;
    }

    void cancel()
    {
        if (cancel_) {
            auto cancel = std::move(cancel_);
            cancel();
        }
    }

    explicit operator bool() const { return static_cast<bool>(cancel_); }

private:
    std::function<void()> cancel_;
};

namespace detail {

template <typename T>
std::string valueToString(const T& value)
{
    std::ostringstream oss;
    if constexpr (requires(std::ostringstream& stream, const T& candidate) {
                      stream << candidate;
                  }) {
        oss << value;
        return oss.str();
    } else {
        return "<" + std::string(typeid(T).name()) + ">";
    }
}

struct ObserverControl {
    std::mutex mutex;
    std::unordered_set<size_t> cancelled;
};

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
    void onRead(const void* source, std::function<ObserverHandle()> registerObserver)
    {
        if (seen_.insert(source).second) {
            if (registerObserver) {
                subscriptions_.push_back(registerObserver());
            }
        }
    }

    void clearSubscriptions()
    {
        for (auto& subscription : subscriptions_) {
            subscription.cancel();
        }
        subscriptions_.clear();
        seen_.clear();
    }

private:
    static inline thread_local ReadTracker* current_ = nullptr;
    std::unordered_set<const void*> seen_;
    std::vector<ObserverHandle> subscriptions_;
    std::function<void()> onInvalidate_;
};

class ReadTrackingScope {
public:
    explicit ReadTrackingScope(ReadTracker* tracker)
        : previous_(ReadTracker::current())
    {
        ReadTracker::begin(tracker);
    }

    ~ReadTrackingScope() { ReadTracker::begin(previous_); }

    ReadTrackingScope(const ReadTrackingScope&) = delete;
    ReadTrackingScope& operator=(const ReadTrackingScope&) = delete;

private:
    ReadTracker* previous_ = nullptr;
};

} // namespace detail

template <typename T>
class MutableState {
public:
    using Observer = std::function<void(const T&)>;

    MutableState(T initial_value)
        : value_(std::move(initial_value))
    {
    }

    T get() const
    {
        if (auto* tracker = detail::ReadTracker::current()) {
            auto cb = tracker->invalidationCallback();
            tracker->onRead(this, [this, cb]() {
                return const_cast<MutableState*>(this)->observe([cb](const T&) {
                    if (cb) {
                        cb();
                    }
                });
            });
        }
        std::lock_guard lock(mutex_);
        return value_;
    }

    void set(T new_value)
    {
        bool changed = false;
        std::string strVal;
        std::vector<ObserverEntry> observers;
        std::function<void(const std::string&)> onChange;
        T newValue;
        {
            std::lock_guard lock(mutex_);
            if (value_ != new_value) {
                value_ = new_value;
                newValue = value_;
                strVal = valueToStringLocked();
                changed = true;
                observers = observers_;
                onChange = onChange_;
            }
        }
        if (changed) {
            for (const auto& entry : observers) {
                if (entry.hasOwner && entry.owner.isNull()) {
                    continue;
                }
                {
                    std::lock_guard lock(observerControl_->mutex);
                    if (observerControl_->cancelled.contains(entry.id)) {
                        continue;
                    }
                }
                try {
                    entry.fn(newValue);
                } catch (...) {
                    // Observer failures must not prevent other observers from receiving the update.
                }
            }
        }
        if (changed && onChange) {
            try {
                onChange(strVal);
            } catch (...) {
                // DevTools observers are auxiliary and must not break state updates.
            }
        }
    }

    ObserverHandle observe(Observer observer)
    {
        std::lock_guard lock(mutex_);
        pruneObserversLocked();
        ObserverEntry entry;
        entry.id = nextObserverId_++;
        entry.fn = std::move(observer);
        const auto id = entry.id;
        observers_.push_back(std::move(entry));
        return makeObserverHandle(id);
    }

    // 绑定 owner 的观察者：owner 销毁后自动跳过通知，避免悬垂指针。
    ObserverHandle observe(QObject* owner, Observer observer)
    {
        std::lock_guard lock(mutex_);
        pruneObserversLocked();
        ObserverEntry entry;
        entry.id = nextObserverId_++;
        entry.owner = QPointer<QObject>(owner);
        entry.hasOwner = true;
        entry.fn = std::move(observer);
        const auto id = entry.id;
        observers_.push_back(std::move(entry));
        return makeObserverHandle(id);
    }

    // 内部：设置 DevTools 变更回调（接收已计算好的字符串值）
    void setOnChange(std::function<void(const std::string&)> cb) { onChange_ = std::move(cb); }

    // 调用者必须已持有锁
    std::string valueToStringLocked() const
    {
        return detail::valueToString(value_);
    }

    // 线程安全版本
    std::string valueToString() const
    {
        std::lock_guard lock(mutex_);
        return detail::valueToString(value_);
    }

private:
    struct ObserverEntry {
        size_t id = 0;
        QPointer<QObject> owner;
        bool hasOwner = false;
        Observer fn;
    };

    mutable std::mutex mutex_;
    std::shared_ptr<detail::ObserverControl> observerControl_ = std::make_shared<detail::ObserverControl>();
    T value_;
    std::vector<ObserverEntry> observers_;
    size_t nextObserverId_ = 1;
    std::function<void(const std::string&)> onChange_;

    ObserverHandle makeObserverHandle(size_t id)
    {
        std::weak_ptr<detail::ObserverControl> weakControl = observerControl_;
        return ObserverHandle([weakControl, id]() {
            if (auto control = weakControl.lock()) {
                std::lock_guard lock(control->mutex);
                control->cancelled.insert(id);
            }
        });
    }

    void pruneObserversLocked()
    {
        std::lock_guard controlLock(observerControl_->mutex);
        observers_.erase(
            std::remove_if(
                observers_.begin(), observers_.end(),
                [&](const ObserverEntry& entry) {
                    return observerControl_->cancelled.contains(entry.id) || (entry.hasOwner && entry.owner.isNull());
                }),
            observers_.end());
    }
};

// 无 tag，不注册 DevTools
template <typename T>
auto mutableStateOf(T initial_value)
{
    return std::make_shared<MutableState<T>>(std::move(initial_value));
}

// 带 tag，自动注册到 StateRegistryMgr（DevTools 可见）
template <typename T>
auto mutableStateOf(const std::string& tag, T initial_value)
{
    auto state = std::make_shared<MutableState<T>>(std::move(initial_value));

    // 注册到 StateRegistryMgr
    StateRegistryMgr::Instance()->registerState(tag, state, [state] {
        return state->valueToString();
    });

    // 变更时通知 StateRegistryMgr（strVal 已在 set() 内计算好，无需再加锁）
    state->setOnChange([tag](const std::string& strVal) {
        StateRegistryMgr::Instance()->notifyChange(tag, strVal);
    });

    return state;
}

} // namespace rui
