#pragma once

#include <QWidget>

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "modifier.hpp"
#include "../core/observable_state.hpp"

namespace rui {

// ── TopAppBar ──

std::unique_ptr<QWidget> TopAppBar(
    std::shared_ptr<MutableState<std::string>> title,
    std::unique_ptr<QWidget> navigationIcon = nullptr,
    std::vector<std::unique_ptr<QWidget>> actions = {},
    Modifier modifier = Modifier{}
);

// ── FloatingActionButton ──

std::unique_ptr<QWidget> FloatingActionButton(
    std::function<void()> onClick,
    std::unique_ptr<QWidget> content,
    Modifier modifier = Modifier{}
);

// ── Snackbar ──

enum class SnackbarDuration { Short, Long, Indefinite };

using SnackbarState = std::shared_ptr<MutableState<std::string>>;

// ── Scaffold ──

std::unique_ptr<QWidget> Scaffold(
    Modifier modifier = Modifier{},
    std::unique_ptr<QWidget> topBar = nullptr,
    std::unique_ptr<QWidget> content = nullptr,
    std::unique_ptr<QWidget> bottomBar = nullptr,
    std::unique_ptr<QWidget> floatingActionButton = nullptr
);

} // namespace rui
