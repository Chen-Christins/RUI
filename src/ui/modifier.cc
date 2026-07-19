// modifier.cc
#include "modifier.hpp"

namespace rui {

void applyModifier(QWidget* widget, const Modifier& modifier) {
    // 应用尺寸
    if (modifier.size_.width > 0 && modifier.size_.height > 0) {
        widget->setFixedSize(modifier.size_.width, modifier.size_.height);
    } else if (modifier.size_.width > 0) {
        widget->setFixedWidth(modifier.size_.width);
    } else if (modifier.size_.height > 0) {
        widget->setFixedHeight(modifier.size_.height);
    }

    // 应用 padding（通过内容边距）
    const auto& p = modifier.padding_;
    if (p.left > 0 || p.top > 0 || p.right > 0 || p.bottom > 0) {
        widget->setContentsMargins(p.left, p.top, p.right, p.bottom);
    }
}

} // namespace rui
