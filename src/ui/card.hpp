// card.hpp
#pragma once

#include "component.hpp"
#include "modifier.hpp"

namespace rui {

Component Card(Component content, Modifier modifier = Modifier{}, Dimension cornerRadius = 8_dp, float elevation = 2.0f);

} // namespace rui
