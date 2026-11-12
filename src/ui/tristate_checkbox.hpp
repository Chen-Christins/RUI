// tristate_checkbox.hpp
#pragma once

#include <functional>
#include <memory>
#include <ostream>

#include "../core/observable_state.hpp"
#include "component.hpp"
#include "modifier.hpp"

namespace rui {

enum class TriState {
    Unchecked,
    Checked,
    Indeterminate,
};

inline std::ostream& operator<<(std::ostream& os, TriState s)
{
    switch (s) {
    case TriState::Unchecked:
        return os << "Unchecked";
    case TriState::Checked:
        return os << "Checked";
    case TriState::Indeterminate:
        return os << "Indeterminate";
    }
    return os << "Unchecked";
}

Component TriStateCheckbox(
    std::shared_ptr<MutableState<TriState>> state,
    std::function<void(TriState)> onStateChange = nullptr,
    bool enabled = true,
    Modifier modifier = Modifier{});

} // namespace rui
