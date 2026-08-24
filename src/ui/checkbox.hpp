// checkbox.hpp
#pragma once

#include <functional>
#include <memory>

#include "component.hpp"
#include "modifier.hpp"
#include "../core/observable_state.hpp"

namespace rui {

Component Checkbox(
    std::shared_ptr<MutableState<bool>> checked,
    std::function<void(bool)> onCheckedChange = nullptr,
    bool enabled = true,
    Modifier modifier = Modifier{}
);

} // namespace rui
