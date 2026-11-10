// animated_visibility.hpp
#pragma once

#include <memory>

#include "../core/observable_state.hpp"
#include "component.hpp"
#include "modifier.hpp"

namespace rui {

// AnimatedVisibility —— 根据 visible 状态淡入淡出内容。
Component AnimatedVisibility(std::shared_ptr<MutableState<bool>> visible, Component content, Modifier modifier = Modifier{});

} // namespace rui
