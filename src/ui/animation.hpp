// animation.hpp
#pragma once

#include <functional>
#include <memory>

#include "../core/observable_state.hpp"

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

// animateFloatAsState —— 返回一个会随时间平滑过渡到 target 的可观察状态。
std::shared_ptr<MutableState<float>> animateFloatAsState(
    float target,
    AnimationSpec spec = AnimationSpec::tween(300),
    std::function<void()> finishedListener = nullptr
);

} // namespace rui
