// macos_window.hpp
#pragma once

class QColor;
class QWidget;

namespace rui {

// 将窗口标题栏设为透明并应用指定背景色，使标题栏颜色跟随内容区背景。
void applyMacWindowBackgroundColor(QWidget* widget, const QColor& color);

} // namespace rui
