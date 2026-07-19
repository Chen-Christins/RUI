// state_registry.hpp
#pragma once

#include <QJsonObject>

#include <functional>
#include <map>
#include <memory>
#include <string>

namespace rui {

// 全局状态注册表 — DevTools 可通过它枚举和监控所有 MutableState
class StateRegistry {
public:
    static StateRegistry& instance();

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
    using ChangeCallback = std::function<void(const std::string& tag, const std::string& value)>;
    void setChangeCallback(ChangeCallback callback);

private:
    StateRegistry() = default;

private:
    struct StateInfo {
        std::shared_ptr<void> ptr;          // weak 引用实际状态对象
        std::function<std::string()> getter; // 读取当前值
    };

    std::map<std::string, StateInfo> states_;
    ChangeCallback changeCallback_;
};

} // namespace rui
