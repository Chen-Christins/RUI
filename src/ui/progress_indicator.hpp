// progress_indicator.hpp
#pragma once

#include <memory>

#include "../core/observable_state.hpp"
#include "color.hpp"
#include "component.hpp"
#include "modifier.hpp"

namespace rui {

Component CircularProgressIndicator(
    std::shared_ptr<MutableState<float>> progress,
    Modifier modifier = Modifier{},
    Color color = Color::blue(),
    Dimension strokeWidth = 4_dp);

Component LinearProgressIndicator(
    std::shared_ptr<MutableState<float>> progress,
    Modifier modifier = Modifier{},
    Color color = Color::blue(),
    Color trackColor = Color::grey200(),
    Dimension strokeWidth = 4_dp);

} // namespace rui
