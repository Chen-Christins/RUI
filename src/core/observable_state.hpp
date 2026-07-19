// observable_state.hpp
#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

#include "devtools/state_registry.hpp"

namespace rui {

template <typename T>
class MutableState {
public:
    using Observer = std::function<void(const T&)>;

    MutableState(T initial_value) : value_(std::move(initial_value)) {}

    T get() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return value_;
    }

    void set(T new_value) {
        bool changed = false;
        std::string strVal;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (value_ != new_value) {
                value_ = std::move(new_value);
                notifyObservers();
                strVal = valueToStringLocked();
                changed = true;
            }
        }
        if (changed && onChange_) {
            onChange_(strVal);
        }
    }

    void observe(Observer observer) {
        std::lock_guard<std::mutex> lock(mutex_);
        observers_.push_back(std::move(observer));
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
        std::lock_guard<std::mutex> lock(mutex_);
        std::ostringstream oss;
        oss << value_;
        return oss.str();
    }

private:
    void notifyObservers() {
        for (const auto& observer : observers_) {
            observer(value_);
        }
    }

private:
    mutable std::mutex mutex_;
    T value_;
    std::vector<Observer> observers_;
    std::function<void(const std::string&)> onChange_;
};

// 无 tag，不注册 DevTools
template <typename T>
auto mutableStateOf(T initial_value) {
    return std::make_shared<MutableState<T>>(std::move(initial_value));
}

// 带 tag，自动注册到 StateRegistry（DevTools 可见）
template <typename T>
auto mutableStateOf(const std::string& tag, T initial_value) {
    auto state = std::make_shared<MutableState<T>>(std::move(initial_value));

    // 注册到 StateRegistry
    StateRegistry::Instance()->registerState(tag, state,
        [state]() { return state->valueToString(); });

    // 变更时通知 StateRegistry（strVal 已在 set() 内计算好，无需再加锁）
    state->setOnChange([tag](const std::string& strVal) {
        StateRegistry::Instance()->notifyChange(tag, strVal);
    });

    return state;
}

} // namespace rui
