#pragma once

#include <QWidget>

#include <functional>
#include <memory>

#include "modifier.hpp"

namespace rui {

std::unique_ptr<QWidget> LazyColumn(
    int itemCount,
    std::function<std::unique_ptr<QWidget>(int index)> factory,
    Modifier modifier = Modifier{}
);

std::unique_ptr<QWidget> LazyRow(
    int itemCount,
    std::function<std::unique_ptr<QWidget>(int index)> factory,
    Modifier modifier = Modifier{}
);

} // namespace rui
