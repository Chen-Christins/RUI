// divider.hpp
#pragma once

#include "color.hpp"
#include "component.hpp"
#include "modifier.hpp"

namespace rui {

Component Divider(Modifier modifier = Modifier{}, Color color = Color::grey300(), Dimension thickness = 1_px);

} // namespace rui
