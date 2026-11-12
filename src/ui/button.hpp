// button.hpp
#pragma once

#include <functional>
#include <memory>
#include <string>

#include "../core/observable_state.hpp"
#include "component.hpp"
#include "modifier.hpp"

namespace rui {

// Button 组件 - 接受 Text 等 RUI 组件作为内容
Component Button(std::function<void()> onClick, Component content, Modifier modifier = Modifier{});

// Button 组件 - 直接接受状态文本（支持动态更新）
Component Button(std::function<void()> onClick, std::shared_ptr<MutableState<std::string>> text, Modifier modifier = Modifier{});

} // namespace rui
