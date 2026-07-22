/**
 * @file util.h
 * @brief Utility functions and definitions for the project.
 * @author Christins (chen.christins@qq.com)
 * @date 2026-07-22
 * @copyright Apache 2.0
 */
#pragma once

#include <QWidget>

#include "../ui/component_type.hpp"

namespace rui::utils {

inline void setComponentType(QWidget* widget, const char* type) {
    widget->setProperty(kComponentTypeProperty, type);
}

} // namespace rui::utils