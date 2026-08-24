// animation.hpp
#pragma once

#include <QAbstractAnimation>
#include <QSequentialAnimationGroup>
#include <QVariantAnimation>

#include <functional>
#include <memory>
#include <utility>

#include "../core/composable.hpp"
#include "../core/observable_state.hpp"
#include "color.hpp"
#include "dimension.hpp"
#include "geometry.hpp"

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
    int repeatCount = 1;      // 播放次数；-1 表示无限循环
    bool reverseRepeat = false; // 是否正反向交替播放

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

    static AnimationSpec repeatable(const AnimationSpec& base, bool reverse = true, int iterations = -1) {
        AnimationSpec s = base;
        s.repeatCount = iterations;
        s.reverseRepeat = reverse;
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

inline QVariant sizeToVariant(Size s) { return QVariant::fromValue(s); }
inline Size sizeFromVariant(const QVariant& v) { return v.value<Size>(); }

inline QVariant offsetToVariant(Offset o) { return QVariant::fromValue(o); }
inline Offset offsetFromVariant(const QVariant& v) { return v.value<Offset>(); }

inline QVariant rectToVariant(Rect r) { return QVariant::fromValue(r); }
inline Rect rectFromVariant(const QVariant& v) { return v.value<Rect>(); }

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

        const T start = state_->get();
        const T finalValue = spec.reverseRepeat ? start : target;

        auto* forward = new QVariantAnimation();
        forward->setStartValue(to_(start));
        forward->setEndValue(to_(target));
        forward->setDuration(spec.durationMillis);
        forward->setEasingCurve(detail::toQtEasing(spec.easing));

        QObject::connect(forward, &QVariantAnimation::valueChanged, [this](const QVariant& v) {
            state_->set(from_(v));
        });

        QAbstractAnimation* driver = forward;

        if (spec.reverseRepeat) {
            auto* backward = new QVariantAnimation();
            backward->setStartValue(to_(target));
            backward->setEndValue(to_(start));
            backward->setDuration(spec.durationMillis);
            backward->setEasingCurve(detail::toQtEasing(spec.easing));

            QObject::connect(backward, &QVariantAnimation::valueChanged, [this](const QVariant& v) {
                state_->set(from_(v));
            });

            auto* seq = new QSequentialAnimationGroup();
            seq->addAnimation(forward);
            seq->addAnimation(backward);
            if (spec.repeatCount != 1) {
                seq->setLoopCount(spec.repeatCount);
            }
            driver = seq;

            QObject::connect(seq, &QAbstractAnimation::finished, [this, finalValue, finished]() {
                state_->set(finalValue);
                if (finished) {
                    finished();
                }
            });
        } else {
            if (spec.repeatCount != 1) {
                forward->setLoopCount(spec.repeatCount);
            }
            QObject::connect(forward, &QVariantAnimation::finished, [this, finalValue, finished]() {
                state_->set(finalValue);
                if (finished) {
                    finished();
                }
            });
        }

        anim_.reset(driver);
        driver->start();
    }

private:
    std::shared_ptr<MutableState<T>> state_;
    Converter to_;
    Unconverter from_;
    std::unique_ptr<QAbstractAnimation> anim_;
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

std::shared_ptr<MutableState<Size>> animateSizeAsState(
    Size target,
    AnimationSpec spec = AnimationSpec::tween(300),
    std::function<void()> finishedListener = nullptr
);

std::shared_ptr<MutableState<Offset>> animateOffsetAsState(
    Offset target,
    AnimationSpec spec = AnimationSpec::tween(300),
    std::function<void()> finishedListener = nullptr
);

std::shared_ptr<MutableState<Rect>> animateRectAsState(
    Rect target,
    AnimationSpec spec = AnimationSpec::tween(300),
    std::function<void()> finishedListener = nullptr
);

// Transition —— 绑定目标状态，随其变化驱动多个属性动画。
template <typename T>
class Transition {
public:
    explicit Transition(std::shared_ptr<MutableState<T>> target) : target_(std::move(target)) {}

    std::shared_ptr<MutableState<float>> animateFloat(
        std::function<float(const T&)> map,
        AnimationSpec spec = AnimationSpec::tween(300),
        std::function<void()> finishedListener = nullptr
    ) {
        return animateFloatAsState(map(target_->get()), spec, std::move(finishedListener));
    }

    std::shared_ptr<MutableState<int>> animateInt(
        std::function<int(const T&)> map,
        AnimationSpec spec = AnimationSpec::tween(300),
        std::function<void()> finishedListener = nullptr
    ) {
        return animateIntAsState(map(target_->get()), spec, std::move(finishedListener));
    }

    std::shared_ptr<MutableState<Color>> animateColor(
        std::function<Color(const T&)> map,
        AnimationSpec spec = AnimationSpec::tween(300),
        std::function<void()> finishedListener = nullptr
    ) {
        return animateColorAsState(map(target_->get()), spec, std::move(finishedListener));
    }

    std::shared_ptr<MutableState<Dimension>> animateDp(
        std::function<Dimension(const T&)> map,
        AnimationSpec spec = AnimationSpec::tween(300),
        std::function<void()> finishedListener = nullptr
    ) {
        return animateDpAsState(map(target_->get()), spec, std::move(finishedListener));
    }

    std::shared_ptr<MutableState<Size>> animateSize(
        std::function<Size(const T&)> map,
        AnimationSpec spec = AnimationSpec::tween(300),
        std::function<void()> finishedListener = nullptr
    ) {
        return animateSizeAsState(map(target_->get()), spec, std::move(finishedListener));
    }

private:
    std::shared_ptr<MutableState<T>> target_;
};

// updateTransition —— 在组合中创建并记住一个 Transition 实例。
template <typename T>
std::shared_ptr<Transition<T>> updateTransition(std::shared_ptr<MutableState<T>> target) {
    if (auto* ctx = ComposableContext::current()) {
        auto& slot = ctx->remember<std::shared_ptr<Transition<T>>>(std::shared_ptr<Transition<T>>());
        if (!slot) {
            slot = std::make_shared<Transition<T>>(std::move(target));
        }
        return slot;
    }
    return std::make_shared<Transition<T>>(std::move(target));
}

// InfiniteTransition —— 无限循环动画。
class InfiniteTransition {
public:
    std::shared_ptr<MutableState<float>> animateFloat(
        float initialValue,
        float targetValue,
        AnimationSpec spec = AnimationSpec::tween(300)
    );
};

std::shared_ptr<InfiniteTransition> rememberInfiniteTransition();

} // namespace rui
