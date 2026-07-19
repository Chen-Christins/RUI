// column.hpp
#pragma once

#include <QWidget>
#include <QVBoxLayout>

#include <memory>

#include "../core/composable.hpp"
#include "modifier.hpp"

namespace rui {

namespace detail {
    template<typename T>
    void addToColumn(QVBoxLayout* layout, T&& child) {
        auto* w = child.release();
        // 读取子组件的 alignment 属性并应用到 layout
        QVariant v = w->property("rui_alignment");
        if (v.isValid()) {
            layout->addWidget(w, 0, toQtAlignment(static_cast<Alignment>(v.toInt())));
        } else {
            layout->addWidget(w);
        }
    }
}

// Column 组件 - 垂直布局
template<typename... Children>
Composable std::unique_ptr<QWidget> Column(Arrangement arrangement, Children&&... children) {
    auto widget = std::make_unique<QWidget>();
    auto* layout = new QVBoxLayout(widget.get());
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    (detail::addToColumn(layout, std::forward<Children>(children)), ...);

    applyArrangement(layout, arrangement, static_cast<int>(sizeof...(Children)));

    return widget;
}

// 默认 Arrangement::Start
template<typename... Children>
Composable std::unique_ptr<QWidget> Column(Children&&... children) {
    return Column(Arrangement::Start, std::forward<Children>(children)...);
}

} // namespace rui
