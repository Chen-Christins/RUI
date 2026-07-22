#pragma once

#include <QWidget>

#include <memory>

#include "../core/composable.hpp"
#include "modifier.hpp"

namespace rui {

// Spacer component - occupies empty space in Row/Column layouts.
Composable std::unique_ptr<QWidget> Spacer(Modifier modifier = Modifier{});

} // namespace rui
