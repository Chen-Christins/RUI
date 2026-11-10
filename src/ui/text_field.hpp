#pragma once

#include <QWidget>
#include <memory>
#include <string>

#include "../core/observable_state.hpp"
#include "component.hpp"
#include "modifier.hpp"

namespace rui {

Component TextField(
    std::shared_ptr<MutableState<std::string>> state,
    Modifier modifier = Modifier{},
    const std::string& placeholder = "",
    const std::string& label = "",
    bool enabled = true,
    bool readOnly = false,
    bool singleLine = true);

Component OutlinedTextField(
    std::shared_ptr<MutableState<std::string>> state,
    Modifier modifier = Modifier{},
    const std::string& placeholder = "",
    const std::string& label = "",
    bool enabled = true,
    bool readOnly = false,
    bool singleLine = true);

} // namespace rui
