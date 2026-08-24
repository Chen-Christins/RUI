// flow.hpp
#pragma once

#include <QWidget>

#include <memory>

#include "../utils/util.hpp"
#include "component.hpp"
#include "component_type.hpp"
#include "flow_layout.hpp"
#include "modifier.hpp"

namespace rui {

namespace {

template <typename T>
void addToFlow(FlowLayout* layout, T&& child) {
    auto* w = child.release();
    layout->addWidget(w);
}

} // namespace

// FlowRow - horizontal flow layout with automatic line wrapping.
template <typename... Children>
Component FlowRow(Arrangement arrangement, Children&&... children) {
    (void)arrangement;
    auto widget = std::make_unique<FlowWidget>(Qt::Horizontal);
    utils::setComponentType(widget.get(), kUIFlowRowProperty);
    auto* layout = widget->flowLayout();

    (addToFlow(layout, std::forward<Children>(children)), ...);

    return widget;
}

template <typename... Children>
Component FlowRow(Children&&... children) {
    return FlowRow(Arrangement::Start, std::forward<Children>(children)...);
}

// FlowColumn - vertical flow layout with automatic column wrapping.
template <typename... Children>
Component FlowColumn(Arrangement arrangement, Children&&... children) {
    (void)arrangement;
    auto widget = std::make_unique<FlowWidget>(Qt::Vertical);
    utils::setComponentType(widget.get(), kUIFlowColumnProperty);
    auto* layout = widget->flowLayout();

    (addToFlow(layout, std::forward<Children>(children)), ...);

    return widget;
}

template <typename... Children>
Component FlowColumn(Children&&... children) {
    return FlowColumn(Arrangement::Start, std::forward<Children>(children)...);
}

} // namespace rui
