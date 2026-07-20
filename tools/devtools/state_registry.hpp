// state_registry.hpp
#pragma once

#include <QJsonObject>

#include <functional>
#include <map>
#include <memory>
#include <string>

#include "utils/singleton.hpp"
#include "utils/noncopyable.hpp"

namespace rui {

// 全局状态注册表 — DevTools 可通过它枚举和监控所有 MutableState
class StateRegistry : public Singleton<StateRegistry>, Noncopyable {
public:
    using ChangeCallback = std::function<void(const std::string& tag, const std::string& value)>;

    // 注册一个状态（由 MutableState 内部调用）
    void registerState(const std::string& tag, std::shared_ptr<void> state,
                       std::function<std::string()> valueGetter);

    // 注销状态
    void unregisterState(const std::string& tag);

    // 状态值变更通知（由 MutableState::set 调用）
    void notifyChange(const std::string& tag, const std::string& newValue);

    // 获取所有已注册状态的快照
    QJsonObject getAllStates() const;

    // 设置变更回调（DevToolsServer 订阅以广播）
    void setChangeCallback(ChangeCallback callback);

private:
    friend class Singleton<StateRegistry>;
    StateRegistry() = default;

private:
    // 状态信息结构体，包含状态指针和获取当前值的函数
    struct StateInfo {
        std::shared_ptr<void> ptr;
        std::function<std::string()> getter;
    };
    // tag -> StateInfo
    std::map<std::string, StateInfo> states_;
    // 状态变更回调
    ChangeCallback changeCallback_;
};

} // namespace rui
