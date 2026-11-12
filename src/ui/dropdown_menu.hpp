// dropdown_menu.hpp
#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "../core/observable_state.hpp"
#include "component.hpp"
#include "modifier.hpp"

namespace rui {

struct DropdownMenuItem {
    std::string label;
    std::function<void()> onClick;
};

Component DropdownMenu(
    std::shared_ptr<MutableState<bool>> expanded,
    std::function<void()> onDismissRequest = nullptr,
    std::vector<DropdownMenuItem> items = {},
    Modifier modifier = Modifier{});

} // namespace rui
