// radio_button.hpp
#pragma once

#include <functional>
#include <memory>

#include "../core/observable_state.hpp"
#include "component.hpp"
#include "modifier.hpp"

namespace rui {

Component RadioButton(
    std::shared_ptr<MutableState<bool>> selected,
    std::function<void(bool)> onSelectedChange = nullptr,
    bool enabled = true,
    Modifier modifier = Modifier{});

} // namespace rui
