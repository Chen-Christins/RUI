// row.hpp
#pragma once

#include <QWidget>
#include <QHBoxLayout>

#include <memory>

#include "../core/composable.hpp"
#include "modifier.hpp"

namespace rui {

namespace detail {

template <typename T>
void addToRow(QHBoxLayout* layout, T&& child) {
    auto* w = child.release();
    const int weight = modifierWeight(w);
    QVariant v = w->property("rui_alignment");
    if (v.isValid()) {
        layout->addWidget(w, weight, toQtAlignment(static_cast<Alignment>(v.toInt())));
    } else {
        layout->addWidget(w, weight);
    }
}

} // namespace detail

// Row 组件 - 水平布局
template <typename... Children>
Composable std::unique_ptr<QWidget> Row(Arrangement arrangement, Children&&... children) {
    auto widget = std::make_unique<QWidget>();
    detail::setComponentType(widget.get(), "Row");
    auto* layout = new QHBoxLayout(widget.get());
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    (detail::addToRow(layout, std::forward<Children>(children)), ...);

    applyArrangement(layout, arrangement, static_cast<int>(sizeof...(Children)));

    return widget;
}

// 默认 Arrangement::Start
template <typename... Children>
Composable std::unique_ptr<QWidget> Row(Children&&... children) {
    return Row(Arrangement::Start, std::forward<Children>(children)...);
}

} // namespace rui
