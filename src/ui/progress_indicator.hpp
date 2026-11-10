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
    Color color = Color::themePrimary(),
    Dimension strokeWidth = 4_dp);

Component LinearProgressIndicator(
    std::shared_ptr<MutableState<float>> progress,
    Modifier modifier = Modifier{},
    Color color = Color::themePrimary(),
    Color trackColor = Color::themeTrack(),
    Dimension strokeWidth = 4_dp);

} // namespace rui
