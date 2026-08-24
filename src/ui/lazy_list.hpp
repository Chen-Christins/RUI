#pragma once

#include <functional>
#include <memory>

#include "component.hpp"
#include "modifier.hpp"

namespace rui {

Component LazyColumn(
    int itemCount,
    std::function<Component(int index)> factory,
    Modifier modifier = Modifier{}
);

Component LazyRow(
    int itemCount,
    std::function<Component(int index)> factory,
    Modifier modifier = Modifier{}
);

} // namespace rui
