// animation.cc
#include "animation.hpp"

#include <qpoint.h>

#include <QColor>
#include <QEasingCurve>
#include <QVariantAnimation>

#include "../core/composable.hpp"

namespace rui {

namespace detail {

QEasingCurve toQtEasing(Easing e) {
    switch (e) {
    case Easing::Linear:
        return QEasingCurve(QEasingCurve::Linear);
    case Easing::FastOutSlowIn: {
        QEasingCurve curve(QEasingCurve::BezierSpline);
        curve.addCubicBezierSegment(QPointF(0.4, 0.0), QPointF(0.2, 1.0), QPointF(1.0, 1.0));
        return curve;
    }
    case Easing::FastOutLinearIn:
        return QEasingCurve(QEasingCurve::InCubic);
    case Easing::LinearOutSlowIn:
        return QEasingCurve(QEasingCurve::OutCubic);
    case Easing::EaseIn:
        return QEasingCurve(QEasingCurve::InQuad);
    case Easing::EaseOut:
        return QEasingCurve(QEasingCurve::OutQuad);
    case Easing::EaseInOut:
        return QEasingCurve(QEasingCurve::InOutQuad);
    case Easing::Spring:
        return QEasingCurve(QEasingCurve::OutBack);
    }
    return QEasingCurve(QEasingCurve::Linear);
}

} // namespace detail

namespace {

// 注册 QColor 插值器，使 QVariantAnimation 能插值 QColor。
QVariant interpolateQColor(const QColor& from, const QColor& to, qreal progress) {
    return QVariant(QColor::fromRgbF(
        from.redF() + (to.redF() - from.redF()) * progress,
        from.greenF() + (to.greenF() - from.greenF()) * progress,
        from.blueF() + (to.blueF() - from.blueF()) * progress,
        from.alphaF() + (to.alphaF() - from.alphaF()) * progress
    ));
}

QVariant interpolateSize(const Size& from, const Size& to, qreal progress) {
    return QVariant::fromValue(lerp(from, to, static_cast<float>(progress)));
}

QVariant interpolateOffset(const Offset& from, const Offset& to, qreal progress) {
    return QVariant::fromValue(lerp(from, to, static_cast<float>(progress)));
}

QVariant interpolateRect(const Rect& from, const Rect& to, qreal progress) {
    return QVariant::fromValue(lerp(from, to, static_cast<float>(progress)));
}

const bool kInterpolatorsRegistered = []() {
    qRegisterMetaType<Size>("rui::Size");
    qRegisterMetaType<Offset>("rui::Offset");
    qRegisterMetaType<Rect>("rui::Rect");
    qRegisterAnimationInterpolator<QColor>(&interpolateQColor);
    qRegisterAnimationInterpolator<Size>(&interpolateSize);
    qRegisterAnimationInterpolator<Offset>(&interpolateOffset);
    qRegisterAnimationInterpolator<Rect>(&interpolateRect);
    return true;
}();

template <typename T>
std::shared_ptr<Animatable<T>> rememberAnimatable(
    T initial,
    typename Animatable<T>::Converter to,
    typename Animatable<T>::Unconverter from
) {
    if (auto* ctx = ComposableContext::current()) {
        auto& slot = ctx->remember<std::shared_ptr<Animatable<T>>>(std::shared_ptr<Animatable<T>>());
        if (!slot) {
            slot = std::make_shared<Animatable<T>>(initial, std::move(to), std::move(from));
        }
        return slot;
    }
    return std::make_shared<Animatable<T>>(initial, std::move(to), std::move(from));
}

} // namespace

std::shared_ptr<MutableState<float>> animateFloatAsState(
    float target,
    AnimationSpec spec,
    std::function<void()> finishedListener
) {
    auto animatable = rememberAnimatable<float>(target, detail::floatToVariant, detail::floatFromVariant);
    animatable->animateTo(target, spec, std::move(finishedListener));
    return animatable->state();
}

std::shared_ptr<MutableState<int>> animateIntAsState(
    int target,
    AnimationSpec spec,
    std::function<void()> finishedListener
) {
    auto animatable = rememberAnimatable<int>(target, detail::intToVariant, detail::intFromVariant);
    animatable->animateTo(target, spec, std::move(finishedListener));
    return animatable->state();
}

std::shared_ptr<MutableState<Color>> animateColorAsState(
    Color target,
    AnimationSpec spec,
    std::function<void()> finishedListener
) {
    auto animatable = rememberAnimatable<Color>(target, detail::colorToVariant, detail::colorFromVariant);
    animatable->animateTo(target, spec, std::move(finishedListener));
    return animatable->state();
}

std::shared_ptr<MutableState<Dimension>> animateDpAsState(
    Dimension target,
    AnimationSpec spec,
    std::function<void()> finishedListener
) {
    auto animatable = rememberAnimatable<Dimension>(target, detail::dimensionToVariant, detail::dimensionFromVariant);
    animatable->animateTo(target, spec, std::move(finishedListener));
    return animatable->state();
}

std::shared_ptr<MutableState<Size>> animateSizeAsState(
    Size target,
    AnimationSpec spec,
    std::function<void()> finishedListener
) {
    auto animatable = rememberAnimatable<Size>(target, detail::sizeToVariant, detail::sizeFromVariant);
    animatable->animateTo(target, spec, std::move(finishedListener));
    return animatable->state();
}

std::shared_ptr<MutableState<Offset>> animateOffsetAsState(
    Offset target,
    AnimationSpec spec,
    std::function<void()> finishedListener
) {
    auto animatable = rememberAnimatable<Offset>(target, detail::offsetToVariant, detail::offsetFromVariant);
    animatable->animateTo(target, spec, std::move(finishedListener));
    return animatable->state();
}

std::shared_ptr<MutableState<Rect>> animateRectAsState(
    Rect target,
    AnimationSpec spec,
    std::function<void()> finishedListener
) {
    auto animatable = rememberAnimatable<Rect>(target, detail::rectToVariant, detail::rectFromVariant);
    animatable->animateTo(target, spec, std::move(finishedListener));
    return animatable->state();
}

std::shared_ptr<MutableState<float>> InfiniteTransition::animateFloat(
    float initialValue,
    float targetValue,
    AnimationSpec spec
) {
    auto animatable = rememberAnimatable<float>(initialValue, detail::floatToVariant, detail::floatFromVariant);
    animatable->animateTo(targetValue, AnimationSpec::repeatable(spec, /*reverse=*/true, /*iterations=*/-1), nullptr);
    return animatable->state();
}

std::shared_ptr<InfiniteTransition> rememberInfiniteTransition() {
    if (auto* ctx = ComposableContext::current()) {
        auto& slot = ctx->remember<std::shared_ptr<InfiniteTransition>>(std::shared_ptr<InfiniteTransition>());
        if (!slot) {
            slot = std::make_shared<InfiniteTransition>();
        }
        return slot;
    }
    return std::make_shared<InfiniteTransition>();
}

} // namespace rui
