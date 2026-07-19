// state_registry.cc
#include "state_registry.hpp"

#include <QJsonObject>

namespace rui {

void StateRegistry::registerState(const std::string& tag, std::shared_ptr<void> state, std::function<std::string()> getter) {
    // Qt 主线程运行，无需额外加锁
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
