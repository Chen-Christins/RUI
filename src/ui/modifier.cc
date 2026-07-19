// modifier.cc
#include "modifier.hpp"

#include <QString>

namespace rui {

// ── Modifier 链式设置 ─────────────────────────────────

Modifier& Modifier::padding(int left, int top, int right, int bottom) {
    padding_ = {left, top, right, bottom};
    return *this;
}

Modifier& Modifier::size(int width, int height) {
    size_ = {width, height};
    return *this;
}

Modifier& Modifier::foreground(Color color) {
    foreground_ = color;
    foregroundSet_ = true;
    return *this;
}

Modifier& Modifier::background(Color color) {
    background_ = color;
    backgroundSet_ = true;
    return *this;
}

Modifier& Modifier::fontSize(int px) {
    fontSize_ = px;
    return *this;
}

Modifier& Modifier::border(Color color, int width) {
    borderColor_ = color;
    borderWidth_ = width;
    return *this;
}

// ── applyModifier ─────────────────────────────────────

void applyModifier(QWidget* widget, const Modifier& modifier) {
    if (modifier.size_.width > 0 && modifier.size_.height > 0) {
        widget->setFixedSize(modifier.size_.width, modifier.size_.height);
    } else if (modifier.size_.width > 0) {
        widget->setFixedWidth(modifier.size_.width);
    } else if (modifier.size_.height > 0) {
        widget->setFixedHeight(modifier.size_.height);
    }

    const auto& m = modifier;
    bool hasStyle = m.padding_.left   > 0 || m.padding_.top    > 0 ||
                    m.padding_.right  > 0 || m.padding_.bottom > 0 ||
                    m.foregroundSet_ ||
                    m.backgroundSet_ ||
                    m.fontSize_       > 0 ||
                    m.borderWidth_    > 0;

    if (!hasStyle) {
        return;
    }

    const char* cls = widget->metaObject()->className();
    QString style;

    if (m.padding_.left > 0 || m.padding_.top > 0 || m.padding_.right > 0 || m.padding_.bottom > 0) {
        style += QString("padding: %1px %2px %3px %4px;")
                     .arg(m.padding_.top).arg(m.padding_.right)
                     .arg(m.padding_.bottom).arg(m.padding_.left);
    }

    if (m.foregroundSet_) {
        style += QString("color: %1;").arg(m.foreground_.toQColor().name(QColor::HexArgb));
    }

    if (m.backgroundSet_) {
        style += QString("background: %1;").arg(m.background_.toQColor().name(QColor::HexArgb));
    }

    if (m.fontSize_ > 0) {
        style += QString("font-size: %1px;").arg(m.fontSize_);
    }

    if (m.borderWidth_ > 0) {
        style += QString("border: %1px solid %2;")
                     .arg(m.borderWidth_)
                     .arg(m.borderColor_.toQColor().name(QColor::HexArgb));
    } else {
        style += "border: none;";
    }

    if (!style.isEmpty()) {
        // 合并已有 stylesheet（保留未覆盖的属性，如从内部 Text 继承的颜色）
        QString old = widget->styleSheet();
        if (!old.isEmpty()) {
            // 去掉旧的选择器包装，提取纯属性
            int brace1 = old.indexOf('{');
            int brace2 = old.lastIndexOf('}');
            if (brace1 >= 0 && brace2 > brace1) {
                QString oldProps = old.mid(brace1 + 1, brace2 - brace1 - 1).trimmed();
                if (!oldProps.isEmpty()) {
                    // 旧属性放前面，新属性放后面（新覆盖旧）
                    style = oldProps + " " + style;
                }
            }
        }
        widget->setStyleSheet(QString("%1 { %2 }").arg(cls, style));
    }
}

} // namespace rui
