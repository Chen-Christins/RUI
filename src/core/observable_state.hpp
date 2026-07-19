// observable_state.hpp
#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <vector>

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
        std::lock_guard<std::mutex> lock(mutex_);
        if (value_ != new_value) {
            value_ = std::move(new_value);
            notifyObservers();
        }
    }

    void observe(Observer observer) {
        std::lock_guard<std::mutex> lock(mutex_);
        observers_.push_back(std::move(observer));
    }

private:
    void notifyObservers() {
        for (const auto& observer : observers_) {
            observer(value_);
        }
    }

    mutable std::mutex mutex_;
    T value_;
    std::vector<Observer> observers_;
};

template <typename T>
auto mutableStateOf(T initial_value) {
    return std::make_shared<MutableState<T>>(std::move(initial_value));
}

} // namespace rui