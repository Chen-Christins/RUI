#pragma once

#include <QWidget>

#include <memory>
#include <string>

#include "modifier.hpp"
#include "../core/observable_state.hpp"

namespace rui {

std::unique_ptr<QWidget> TextField(
    std::shared_ptr<MutableState<std::string>> state,
    Modifier modifier = Modifier{},
    const std::string& placeholder = "",
    const std::string& label = "",
    bool enabled = true,
    bool readOnly = false,
    bool singleLine = true
);

std::unique_ptr<QWidget> OutlinedTextField(
    std::shared_ptr<MutableState<std::string>> state,
    Modifier modifier = Modifier{},
    const std::string& placeholder = "",
    const std::string& label = "",
    bool enabled = true,
    bool readOnly = false,
    bool singleLine = true
);

} // namespace rui
