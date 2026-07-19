// row.hpp
#pragma once

#include <QWidget>
#include <QHBoxLayout>

#include <memory>

#include "../core/composable.hpp"

namespace rui {

// Row 组件 - 水平布局（模板，header-only）
template<typename... Children>
Composable std::unique_ptr<QWidget> Row(Children&&... children) {
    auto widget = std::make_unique<QWidget>();
    auto* layout = new QHBoxLayout(widget.get());
    layout->setContentsMargins(0, 0, 0, 0);   // 无边距
    layout->setSpacing(0);                      // 无间距
    layout->setAlignment(Qt::AlignLeft);        // 左对齐

    // 将每个子 widget 添加到布局中
    (layout->addWidget(children.release()), ...);

    return widget;
}

} // namespace rui
