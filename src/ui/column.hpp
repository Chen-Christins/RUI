// column.hpp
#pragma once

#include <QWidget>
#include <QVBoxLayout>

#include <memory>

#include "../core/composable.hpp"

namespace rui {

// Column 组件 - 垂直布局（模板，header-only）
template<typename... Children>
Composable std::unique_ptr<QWidget> Column(Children&&... children) {
    auto widget = std::make_unique<QWidget>();
    auto* layout = new QVBoxLayout(widget.get());

    // 将每个子 widget 添加到布局中
    (layout->addWidget(children.release()), ...);

    return widget;
}

} // namespace rui
