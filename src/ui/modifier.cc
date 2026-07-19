// modifier.cc
#include "modifier.hpp"

#include <QBoxLayout>
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
    foreground_    = color;
    foregroundSet_ = true;
    return *this;
}

Modifier& Modifier::background(Color color) {
    background_    = color;
    backgroundSet_ = true;
    return *this;
}

Modifier& Modifier::fontSize(int px) {
    fontSize_ = px;
    return *this;
}

Modifier& Modifier::border(Color color, int width) {
    borderColor_ = color;
    borderWidth_  = width;
    return *this;
}

Modifier& Modifier::align(Alignment a) {
    alignment_    = a;
    alignmentSet_ = true;
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

    // 对齐（存为 property，Column/Row 读取并应用到 layout）
    if (modifier.alignmentSet_) {
        widget->setProperty("rui_alignment", static_cast<int>(modifier.alignment_));
    }

    const auto& m = modifier;
    bool hasStyle = m.padding_.left   > 0 || m.padding_.top    > 0 ||
                    m.padding_.right  > 0 || m.padding_.bottom > 0 ||
                    m.foregroundSet_ ||
                    m.backgroundSet_ ||
                    m.fontSize_       > 0 ||
                    m.borderWidth_    > 0;

    if (!hasStyle) return;

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
        QString old = widget->styleSheet();
        if (!old.isEmpty()) {
            int b1 = old.indexOf('{');
            int b2 = old.lastIndexOf('}');
            if (b1 >= 0 && b2 > b1) {
                style = old.mid(b1 + 1, b2 - b1 - 1).trimmed() + " " + style;
            }
        }
        widget->setStyleSheet(QString("%1 { %2 }").arg(cls, style));
    }
}

// ── Alignment → Qt ───────────────────────────────────

Qt::Alignment toQtAlignment(Alignment a) {
    switch (a) {
    case Alignment::TopStart:     return Qt::AlignTop | Qt::AlignLeft;
    case Alignment::TopCenter:    return Qt::AlignTop | Qt::AlignHCenter;
    case Alignment::TopEnd:       return Qt::AlignTop | Qt::AlignRight;
    case Alignment::CenterStart:  return Qt::AlignVCenter | Qt::AlignLeft;
    case Alignment::Center:       return Qt::AlignCenter;
    case Alignment::CenterEnd:    return Qt::AlignVCenter | Qt::AlignRight;
    case Alignment::BottomStart:  return Qt::AlignBottom | Qt::AlignLeft;
    case Alignment::BottomCenter: return Qt::AlignBottom | Qt::AlignHCenter;
    case Alignment::BottomEnd:    return Qt::AlignBottom | Qt::AlignRight;
    }
    return Qt::AlignTop | Qt::AlignLeft;
}

// ── Arrangement 实现 ─────────────────────────────────

static void insertStretches(QBoxLayout* layout, Arrangement a, int itemCount) {
    if (itemCount <= 0) return;

    // 先移除旧的 stretch
    for (int i = layout->count() - 1; i >= 0; --i) {
        if (layout->itemAt(i)->spacerItem()) {
            delete layout->takeAt(i);
        }
    }

    auto addStretch = [layout](int stretch = 1) {
        layout->addStretch(stretch);
    };

    switch (a) {
    case Arrangement::Start:
        addStretch(0);  // 末尾加零拉伸，把子组件推到开头
        break;
    case Arrangement::End:
        layout->insertStretch(0, 0);  // 开头加零拉伸
        addStretch(1);
        break;
    case Arrangement::Center:
        layout->insertStretch(0, 1);
        addStretch(1);
        break;
    case Arrangement::SpaceBetween:
        if (itemCount > 1) {
            for (int i = 1; i < itemCount; i++) {
                layout->insertStretch(i * 2 - 1, 1);
            }
        }
        break;
    case Arrangement::SpaceEvenly:
        for (int i = 0; i <= itemCount; i++) {
            layout->insertStretch(i * 2, 1);
        }
        break;
    case Arrangement::SpaceAround:
        for (int i = 0; i <= itemCount; i++) {
            int factor = (i == 0 || i == itemCount) ? 1 : 2;
            layout->insertStretch(i * 2, factor);
        }
        break;
    }
}

void applyArrangement(QVBoxLayout* layout, Arrangement a, int itemCount) {
    insertStretches(layout, a, itemCount);
}

void applyArrangement(QHBoxLayout* layout, Arrangement a, int itemCount) {
    insertStretches(layout, a, itemCount);
}

} // namespace rui
