// window_config.hpp
#pragma once

#include <string>

#include "ui/dimension.hpp"

namespace rui {

struct WindowConfig {
    std::string title;
    std::string key;
    Dimension width = 400_dp;
    Dimension height = 300_dp;
    Dimension minWidth = 0_dp;
    Dimension minHeight = 0_dp;
    Dimension maxWidth = 0_dp;
    Dimension maxHeight = 0_dp;
};

} // namespace rui
