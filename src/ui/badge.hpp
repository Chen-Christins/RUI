// badge.hpp
#pragma once

#include <memory>
#include <string>

#include "color.hpp"
#include "component.hpp"
#include "modifier.hpp"
#include "../core/observable_state.hpp"

namespace rui {

Component Badge(
    std::shared_ptr<MutableState<std::string>> text,
    Modifier modifier = Modifier{},
    Color backgroundColor = Color::red()
);

Component BadgedBox(
    Component badge,
    Modifier modifier = Modifier{},
    Component content = nullptr
);

} // namespace rui
