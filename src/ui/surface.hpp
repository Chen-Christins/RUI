// surface.hpp
#pragma once

#include "color.hpp"
#include "component.hpp"
#include "modifier.hpp"

namespace rui {

Component Surface(
    Component content,
    Modifier modifier = Modifier{},
    Color color = Color::white(),
    RoundedCornerShape shape = RoundedCornerShape(8_dp),
    float elevation = 0.0f);

} // namespace rui
