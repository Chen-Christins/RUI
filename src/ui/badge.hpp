// badge.hpp
#pragma once

#include <memory>
#include <string>

#include "../core/observable_state.hpp"
#include "color.hpp"
#include "component.hpp"
#include "modifier.hpp"

namespace rui {

Component Badge(std::shared_ptr<MutableState<std::string>> text, Modifier modifier = Modifier{}, Color backgroundColor = Color::themeError());

Component BadgedBox(Component badge, Modifier modifier = Modifier{}, Component content = nullptr);

} // namespace rui
