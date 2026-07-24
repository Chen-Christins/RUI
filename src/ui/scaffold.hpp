#pragma once

#include <QWidget>

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "component.hpp"
#include "modifier.hpp"
#include "../core/observable_state.hpp"

namespace rui {

// ── TopAppBar ──

Component TopAppBar(
    std::shared_ptr<MutableState<std::string>> title,
    Component navigationIcon = nullptr,
    std::vector<Component> actions = {},
    Modifier modifier = Modifier{}
);

// ── FloatingActionButton ──

Component FloatingActionButton(
    std::function<void()> onClick,
    Component content,
    Modifier modifier = Modifier{}
);

// ── Snackbar ──

enum class SnackbarDuration { Short, Long, Indefinite };

using SnackbarState = std::shared_ptr<MutableState<std::string>>;

// ── Scaffold ──

Component Scaffold(
    Modifier modifier = Modifier{},
    Component topBar = nullptr,
    Component content = nullptr,
    Component bottomBar = nullptr,
    Component floatingActionButton = nullptr
);

} // namespace rui
