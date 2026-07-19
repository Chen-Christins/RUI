// button.hpp
#pragma once

#include <QPushButton>
#include <QLabel>

#include <memory>
#include <string>
#include <functional>

#include "modifier.hpp"
#include "../core/composable.hpp"
#include "../core/observable_state.hpp"

namespace rui {

// Button 组件 - 接受 QLabel 作为内容（提取文本显示）
Composable std::unique_ptr<QPushButton> Button(
    std::function<void()> onClick,
    std::unique_ptr<QLabel> content,
    Modifier modifier = Modifier{}
);

// Button 组件 - 直接接受状态文本（支持动态更新）
Composable std::unique_ptr<QPushButton> Button(
    std::function<void()> onClick,
    std::shared_ptr<MutableState<std::string>> text,
    Modifier modifier = Modifier{}
);

} // namespace rui
