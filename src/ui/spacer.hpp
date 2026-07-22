#pragma once

#include <QWidget>

#include <memory>

#include "modifier.hpp"

namespace rui {

// Spacer component - occupies empty space in Row/Column layouts.
std::unique_ptr<QWidget> Spacer(Modifier modifier = Modifier{});

} // namespace rui
