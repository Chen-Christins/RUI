#pragma once

#include <memory>

class QWidget;

namespace rui {

using Component = std::unique_ptr<QWidget>;

} // namespace rui
