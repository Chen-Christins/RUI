// macos_window.mm
#include "macos_window.hpp"

#include <QColor>
#include <QWidget>

#import <AppKit/AppKit.h>

namespace rui {

void applyMacWindowBackgroundColor(QWidget* widget, const QColor& color)
{
    if (widget == nullptr || !color.isValid()) {
        return;
    }

    NSView* view = reinterpret_cast<NSView*>(widget->winId());
    NSWindow* window = [view window];
    if (window == nullptr) {
        return;
    }

    window.titlebarAppearsTransparent = YES;
    window.backgroundColor = [NSColor colorWithRed:color.redF() green:color.greenF() blue:color.blueF() alpha:1.0];
}

} // namespace rui
