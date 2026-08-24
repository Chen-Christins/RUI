// box.hpp
#pragma once

#include <memory>
#include <type_traits>

#include "../core/composable.hpp"
#include "../utils/util.hpp"
#include "component.hpp"
#include "modifier.hpp"
#include "component_type.hpp"

namespace rui {

void layoutBoxChild(QWidget* child);

void applyZOrder(QWidget* box);

class BoxWidget : public QWidget {
public:
    using QWidget::QWidget;

    QSize sizeHint() const override;

protected:
    void resizeEvent(QResizeEvent* event) override;
};

template <typename T>
void addToBox(QWidget* box, T&& child) {
    auto* w = child.release();
    w->setParent(box);
    w->show();
    layoutBoxChild(w);
}

template <typename F>
auto addToBox(QWidget* box, F&& factory) -> std::enable_if_t<std::is_invocable_r_v<Component, std::decay_t<F>>, void> {
    addToBox(box, compose(std::forward<F>(factory)));
}

template <typename... Children>
Component Box(Modifier modifier, Children&&... children) {
    auto widget = std::make_unique<BoxWidget>();
    utils::setComponentType(widget.get(), kUIBoxProperty);
    applyModifier(widget.get(), modifier);

    (addToBox(widget.get(), std::forward<Children>(children)), ...);

    applyZOrder(widget.get());

    return widget;
}

template <typename... Children>
Component Box(Children&&... children) {
    return Box(Modifier{}, std::forward<Children>(children)...);
}

} // namespace rui
