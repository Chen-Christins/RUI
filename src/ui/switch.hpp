// switch.hpp
#pragma once

#include <functional>
#include <memory>

#include "../core/observable_state.hpp"
#include "component.hpp"
#include "modifier.hpp"

namespace rui {

Component Switch(
    std::shared_ptr<MutableState<bool>> checked,
    std::function<void(bool)> onCheckedChange = nullptr,
    bool enabled = true,
    Modifier modifier = Modifier{});

} // namespace rui
