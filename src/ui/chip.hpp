// chip.hpp
#pragma once

#include <functional>
#include <memory>
#include <string>

#include "../core/observable_state.hpp"
#include "component.hpp"
#include "modifier.hpp"

namespace rui {

Component Chip(
    std::shared_ptr<MutableState<std::string>> label,
    std::function<void()> onClick = nullptr,
    Modifier modifier = Modifier{},
    std::function<void()> onClose = nullptr,
    bool enabled = true);

Component FilterChip(
    std::shared_ptr<MutableState<bool>> selected,
    std::shared_ptr<MutableState<std::string>> label,
    std::function<void()> onClick = nullptr,
    Modifier modifier = Modifier{},
    bool enabled = true);

} // namespace rui
