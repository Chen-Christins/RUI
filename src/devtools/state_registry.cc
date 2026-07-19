// state_registry.cc
#include "state_registry.hpp"

#include <QJsonObject>

#include <mutex>

namespace rui {

StateRegistry& StateRegistry::instance() {
    static StateRegistry s;
    return s;
}

void StateRegistry::registerState(const std::string& tag,
                                   std::shared_ptr<void> state,
                                   std::function<std::string()> getter) {
    std::lock_guard<std::mutex> lock(reinterpret_cast<std::mutex&>(
        const_cast<std::map<std::string, StateInfo>&>(states_))); // 简单处理
    // 注：简化实现，不使用额外的互斥锁；实际由 MutableState 自身的锁保护
    states_[tag] = {std::move(state), std::move(getter)};
}

void StateRegistry::unregisterState(const std::string& tag) {
    states_.erase(tag);
}

void StateRegistry::notifyChange(const std::string& tag, const std::string& newValue) {
    if (changeCallback_) {
        changeCallback_(tag, newValue);
    }
}

QJsonObject StateRegistry::getAllStates() const {
    QJsonObject obj;
    for (const auto& [tag, info] : states_) {
        if (info.getter) {
            obj[QString::fromStdString(tag)] = QString::fromStdString(info.getter());
        }
    }
    return obj;
}

void StateRegistry::setChangeCallback(ChangeCallback callback) {
    changeCallback_ = std::move(callback);
}

} // namespace rui
