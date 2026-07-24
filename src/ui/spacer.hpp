#pragma once

#include "component.hpp"
#include "modifier.hpp"

namespace rui {

// Spacer component - occupies empty space in Row/Column layouts.
Component Spacer(Modifier modifier = Modifier{});

} // namespace rui
