// slider.hpp
#pragma once

#include <functional>
#include <memory>

#include "../core/observable_state.hpp"
#include "component.hpp"
#include "modifier.hpp"

namespace rui {

Component Slider(
    std::shared_ptr<MutableState<float>> value,
    std::function<void(float)> onValueChange = nullptr,
    Modifier modifier = Modifier{},
    float valueRangeStart = 0.0f,
    float valueRangeEnd = 1.0f,
    int steps = 0,
    bool enabled = true);

} // namespace rui
