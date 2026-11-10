// text.hpp
#pragma once

#include <memory>
#include <string>

#include "../core/observable_state.hpp"
#include "component.hpp"
#include "modifier.hpp"

namespace rui {

// Text 组件 - 显示文本的标签
Component Text(std::shared_ptr<MutableState<std::string>> text, Modifier modifier = Modifier{});

} // namespace rui
