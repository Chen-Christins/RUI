// animation.cc
#include "animation.hpp"

#include <QEasingCurve>
#include <QVariantAnimation>

#include <cmath>

#include "../core/composable.hpp"

namespace rui {

namespace {

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

constexpr float kEpsilon = 0.0001f;

// 持有动画运行时的可动画 float，跨重组保持同一个实例。
class AnimatableFloat {
public:
    explicit AnimatableFloat(float initial)
        : state_(mutableStateOf(initial)), target_(initial) {}

    std::shared_ptr<MutableState<float>> state() const { return state_; }

    void setTarget(float target, const AnimationSpec& spec, std::function<void()> finished) {
        // 目标未变化时不要重启动画（重组会反复调用 setTarget）。
        if (std::fabs(target - target_) < kEpsilon) {
            if (anim_ && anim_->state() == QAbstractAnimation::Running) {
                return;
            }
            if (std::fabs(state_->get() - target) < kEpsilon) {
                return;
            }
        }

        anim_.reset();
        target_ = target;

        if (spec.durationMillis <= 0) {
            state_->set(target);
            if (finished) {
                finished();
            }
            return;
        }

        const float current = state_->get();

        auto* anim = new QVariantAnimation();
        anim->setStartValue(static_cast<double>(current));
        anim->setEndValue(static_cast<double>(target));
        anim->setDuration(spec.durationMillis);
        anim->setEasingCurve(toQtEasing(spec.easing));

        QObject::connect(anim, &QVariantAnimation::valueChanged, [this](const QVariant& v) {
            state_->set(static_cast<float>(v.toDouble()));
        });
        QObject::connect(anim, &QVariantAnimation::finished, [this, finished]() {
            state_->set(target_);
            if (finished) {
                finished();
            }
        });

        anim_.reset(anim);
        anim->start();
    }

private:
    std::shared_ptr<MutableState<float>> state_;
    std::unique_ptr<QVariantAnimation> anim_;
    float target_ = 0.0f;
};

std::shared_ptr<AnimatableFloat> rememberAnimatable(float initial) {
    if (auto* ctx = ComposableContext::current()) {
        auto& slot = ctx->remember<std::shared_ptr<AnimatableFloat>>(
            std::shared_ptr<AnimatableFloat>()
        );
        if (!slot) {
            slot = std::make_shared<AnimatableFloat>(initial);
        }
        return slot;
    }
    return std::make_shared<AnimatableFloat>(initial);
}

} // namespace

std::shared_ptr<MutableState<float>> animateFloatAsState(
    float target,
    AnimationSpec spec,
    std::function<void()> finishedListener
) {
    auto animatable = rememberAnimatable(target);
    animatable->setTarget(target, spec, std::move(finishedListener));
    return animatable->state();
}

} // namespace rui
