// animation.hpp
#pragma once

#include <QVariantAnimation>

#include <functional>
#include <memory>
#include <utility>

#include "../core/observable_state.hpp"
#include "color.hpp"
#include "dimension.hpp"

namespace rui {

enum class Easing {
    Linear,
    FastOutSlowIn,
    FastOutLinearIn,
    LinearOutSlowIn,
    EaseIn,
    EaseOut,
    EaseInOut,
    Spring,
};

class AnimationSpec {
public:
    int durationMillis = 300;
    Easing easing = Easing::FastOutSlowIn;

    static AnimationSpec tween(int durationMillis, Easing easing = Easing::FastOutSlowIn) {
        AnimationSpec s;
        s.durationMillis = durationMillis;
        s.easing = easing;
        return s;
    }

    static AnimationSpec spring(float dampingRatio = 1.0f, float stiffness = 0.0f) {
        (void)dampingRatio;
        (void)stiffness;
        AnimationSpec s;
        s.durationMillis = 300;
        s.easing = Easing::Spring;
        return s;
    }

    static AnimationSpec snap() {
        AnimationSpec s;
        s.durationMillis = 0;
        s.easing = Easing::Linear;
        return s;
    }
};

namespace detail {

QEasingCurve toQtEasing(Easing e);

inline QVariant floatToVariant(float v) { return QVariant(static_cast<double>(v)); }
inline float floatFromVariant(const QVariant& v) { return static_cast<float>(v.toDouble()); }

inline QVariant intToVariant(int v) { return QVariant(v); }
inline int intFromVariant(const QVariant& v) { return v.toInt(); }

inline QVariant colorToVariant(Color c) { return QVariant(c.toQColor()); }
inline Color colorFromVariant(const QVariant& v) { return Color(v.value<QColor>()); }

inline QVariant dimensionToVariant(Dimension d) { return QVariant(d.value()); }
inline Dimension dimensionFromVariant(const QVariant& v) { return Dimension(v.toInt(), DimensionUnit::Dp); }

} // namespace detail

// Animatable —— 可随时间插值过渡到目标值的可观察状态。
template <typename T>
class Animatable {
public:
    using Converter = std::function<QVariant(const T&)>;
    using Unconverter = std::function<T(const QVariant&)>;

    Animatable(T initial, Converter to, Unconverter from)
        : state_(mutableStateOf(initial)), to_(std::move(to)), from_(std::move(from)), target_(initial) {}

    std::shared_ptr<MutableState<T>> state() const { return state_; }

    void animateTo(T target, const AnimationSpec& spec, std::function<void()> finished) {
        // 目标未变化时不要重启动画（重组会反复调用 animateTo）。
        if (target == target_) {
            if (anim_ && anim_->state() == QAbstractAnimation::Running) {
                return;
            }
            if (state_->get() == target) {
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

        auto* anim = new QVariantAnimation();
        anim->setStartValue(to_(state_->get()));
        anim->setEndValue(to_(target));
        anim->setDuration(spec.durationMillis);
        anim->setEasingCurve(detail::toQtEasing(spec.easing));

        QObject::connect(anim, &QVariantAnimation::valueChanged, [this](const QVariant& v) {
            state_->set(from_(v));
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
    std::shared_ptr<MutableState<T>> state_;
    Converter to_;
    Unconverter from_;
    std::unique_ptr<QVariantAnimation> anim_;
    T target_;
};

// animateFloatAsState —— 返回一个会随时间平滑过渡到 target 的可观察状态。
std::shared_ptr<MutableState<float>> animateFloatAsState(
    float target,
    AnimationSpec spec = AnimationSpec::tween(300),
    std::function<void()> finishedListener = nullptr
);

std::shared_ptr<MutableState<int>> animateIntAsState(
    int target,
    AnimationSpec spec = AnimationSpec::tween(300),
    std::function<void()> finishedListener = nullptr
);

std::shared_ptr<MutableState<Color>> animateColorAsState(
    Color target,
    AnimationSpec spec = AnimationSpec::tween(300),
    std::function<void()> finishedListener = nullptr
);

std::shared_ptr<MutableState<Dimension>> animateDpAsState(
    Dimension target,
    AnimationSpec spec = AnimationSpec::tween(300),
    std::function<void()> finishedListener = nullptr
);

} // namespace rui
