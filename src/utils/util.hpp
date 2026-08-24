/**
 * @file util.h
 * @brief Utility functions and definitions for the project.
 * @author Christins (chen.christins@qq.com)
 * @date 2026-07-22
 * @copyright Apache 2.0
 */
#pragma once

#include <QWidget>
#include <QJsonObject>

namespace rui::utils {

void setComponentType(QWidget* widget, const char* type);

QString widgetQtType(QWidget* widget);

QString widgetComponentType(QWidget* widget);

QJsonObject colorToJson(const QColor& c);

QJsonObject dpDimension(int value, const QWidget* context);

QJsonObject pxDimension(int value, const QWidget* context);

} // namespace rui::utils