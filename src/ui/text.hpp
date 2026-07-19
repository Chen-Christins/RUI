// text.hpp
#pragma once

#include <QLabel>

#include <memory>
#include <string>

#include "modifier.hpp"
#include "../core/composable.hpp"
#include "../core/observable_state.hpp"

namespace rui {

// Text 组件 - 显示文本的标签
Composable std::unique_ptr<QLabel> Text(
    std::shared_ptr<MutableState<std::string>> text,
    Modifier modifier = Modifier{}
);

} // namespace rui
