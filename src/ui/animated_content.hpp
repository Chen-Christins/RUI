// animated_content.hpp
#pragma once

#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QStackedLayout>

#include <memory>
#include <utility>

#include "../core/observable_state.hpp"
#include "../utils/util.hpp"
#include "component.hpp"
#include "component_type.hpp"
#include "modifier.hpp"

namespace rui {

// AnimatedContent —— 目标状态变化时，新旧内容交叉淡入淡出。
template <typename T, typename ContentFn>
class AnimatedContentWidget : public QWidget {
public:
    AnimatedContentWidget(std::shared_ptr<MutableState<T>> target, ContentFn content)
        : QWidget(nullptr), target_(std::move(target)), content_(std::move(content)) {
        utils::setComponentType(this, kUIAnimatedContentProperty);
        setAttribute(Qt::WA_TranslucentBackground);

        layout_ = new QStackedLayout(this);
        layout_->setContentsMargins(0, 0, 0, 0);
        layout_->setStackingMode(QStackedLayout::StackAll);

        current_ = buildLayer(target_->get());

        target_->observe([this](const T& value) {
            onTargetChanged(value);
        });
    }

private:
    struct Layer {
        QWidget* widget = nullptr;
        QGraphicsOpacityEffect* effect = nullptr;
    };

    Layer buildLayer(const T& value) {
        Component c = content_(value);
        if (!c) {
            c = std::make_unique<QWidget>();
        }
        auto* w = c.release();
        auto* effect = new QGraphicsOpacityEffect(w);
        effect->setOpacity(1.0);
        w->setGraphicsEffect(effect);
        layout_->addWidget(w);
        return Layer{w, effect};
    }

    void onTargetChanged(const T& value) {
        Layer incoming = buildLayer(value);
        if (!current_.widget) {
            current_ = incoming;
            return;
        }

        Layer outgoing = current_;
        current_ = incoming;

        auto* fadeIn = new QPropertyAnimation(incoming.effect, "opacity", this);
        fadeIn->setStartValue(0.0);
        fadeIn->setEndValue(1.0);
        fadeIn->setDuration(200);
        fadeIn->setEasingCurve(QEasingCurve::InOutCubic);

        auto* fadeOut = new QPropertyAnimation(outgoing.effect, "opacity", this);
        fadeOut->setStartValue(1.0);
        fadeOut->setEndValue(0.0);
        fadeOut->setDuration(200);
        fadeOut->setEasingCurve(QEasingCurve::InOutCubic);

        QObject::connect(fadeOut, &QPropertyAnimation::finished, this, [outgoing]() {
            if (outgoing.widget) {
                outgoing.widget->deleteLater();
            }
        });

        fadeIn->start();
        fadeOut->start();
    }

private:
    std::shared_ptr<MutableState<T>> target_;
    ContentFn content_;
    QStackedLayout* layout_ = nullptr;
    Layer current_;
};

template <typename T, typename ContentFn>
Component AnimatedContent(
    std::shared_ptr<MutableState<T>> targetState,
    ContentFn&& content,
    Modifier modifier = Modifier{}
) {
    auto widget = std::make_unique<AnimatedContentWidget<T, std::decay_t<ContentFn>>>(
        std::move(targetState), std::forward<ContentFn>(content));
    applyModifier(widget.get(), modifier);
    return widget;
}

// Crossfade —— AnimatedContent 的默认淡入淡出别名。
template <typename T, typename ContentFn>
Component Crossfade(
    std::shared_ptr<MutableState<T>> targetState,
    ContentFn&& content,
    Modifier modifier = Modifier{}
) {
    return AnimatedContent(std::move(targetState), std::forward<ContentFn>(content), modifier);
}

} // namespace rui
