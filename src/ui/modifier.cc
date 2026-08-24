// modifier.cc
#include "modifier.hpp"

#include <QBoxLayout>
#include <QCursor>
#include <QGraphicsDropShadowEffect>
#include <QMouseEvent>
#include <QPainterPath>
#include <QRegion>
#include <QSizePolicy>
#include <QString>

#include "component_type.hpp"

namespace rui {

namespace {

class ClickFilter : public QObject {
public:
    explicit ClickFilter(std::function<void()> onClick, QObject* parent = nullptr)
        : QObject(parent), onClick_(std::move(onClick)) {}

protected:
    bool eventFilter(QObject* watched, QEvent* event) override {
        if (event->type() != QEvent::MouseButtonRelease) {
            return false;
        }

        auto* mouse = static_cast<QMouseEvent*>(event);
        if (mouse->button() != Qt::LeftButton || !onClick_) {
            return false;
        }

        if (auto* widget = qobject_cast<QWidget*>(watched)) {
            if (!widget->rect().contains(mouse->pos())) {
                return false;
            }
        }

        onClick_();
        return true;
    }

private:
    std::function<void()> onClick_;
};

class RoundedClipFilter : public QObject {
public:
    explicit RoundedClipFilter(int radius, QObject* parent = nullptr)
        : QObject(parent), radius_(radius) {}

protected:
    bool eventFilter(QObject* watched, QEvent* event) override {
        if (event->type() == QEvent::Resize || event->type() == QEvent::Show) {
            if (auto* widget = qobject_cast<QWidget*>(watched)) {
                QPainterPath path;
                path.addRoundedRect(widget->rect(), radius_, radius_);
                widget->setMask(QRegion(path.toFillPolygon().toPolygon()));
            }
        }
        return false;
    }

private:
    int radius_;
};

} // namespace

Modifier& Modifier::padding(Dimension all) {
    return padding(all, all, all, all);
}

Modifier& Modifier::padding(Dimension horizontal, Dimension vertical) {
    return padding(horizontal, vertical, horizontal, vertical);
}

Modifier& Modifier::padding(Dimension left, Dimension top, Dimension right, Dimension bottom) {
    padding_ = {left, top, right, bottom};
    return *this;
}

Modifier& Modifier::size(Dimension width, Dimension height) {
    size_ = {width, height};
    return *this;
}

Modifier& Modifier::width(Dimension width) {
    size_.width = width;
    return *this;
}

Modifier& Modifier::height(Dimension height) {
    size_.height = height;
    return *this;
}

Modifier& Modifier::fillMaxWidth() {
    fillMaxWidth_ = true;
    return *this;
}

Modifier& Modifier::fillMaxHeight() {
    fillMaxHeight_ = true;
    return *this;
}

Modifier& Modifier::fillMaxSize() {
    return fillMaxWidth().fillMaxHeight();
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

Modifier& Modifier::background(Color color, RoundedCornerShape shape) {
    background(color);
    cornerRadius_ = shape.radius;
    cornerRadiusSet_ = true;
    return *this;
}

Modifier& Modifier::fontSize(Dimension size) {
    fontSize_ = size;
    return *this;
}

Modifier& Modifier::border(Color color, Dimension width) {
    borderColor_ = color;
    borderWidth_  = width;
    return *this;
}

Modifier& Modifier::border(Color color, Dimension width, RoundedCornerShape shape) {
    border(color, width);
    cornerRadius_ = shape.radius;
    cornerRadiusSet_ = true;
    return *this;
}

Modifier& Modifier::clip(RoundedCornerShape shape) {
    cornerRadius_ = shape.radius;
    cornerRadiusSet_ = true;
    clipSet_ = true;
    return *this;
}

Modifier& Modifier::clickable(std::function<void()> onClick) {
    onClick_ = std::move(onClick);
    return *this;
}

Modifier& Modifier::weight(int weight) {
    weight_ = weight;
    return *this;
}

Modifier& Modifier::align(Alignment a) {
    alignment_    = a;
    alignmentSet_ = true;
    return *this;
}

Modifier& Modifier::alpha(float opacity) {
    alpha_    = opacity;
    alphaSet_ = true;
    return *this;
}

Modifier& Modifier::offset(Dimension x, Dimension y) {
    offsetX_    = x;
    offsetY_    = y;
    offsetSet_  = true;
    return *this;
}

Modifier& Modifier::shadow(Dimension elevation, RoundedCornerShape shape) {
    (void)shape;
    shadowElevation_ = elevation;
    shadowSet_       = true;
    return *this;
}

Modifier& Modifier::zIndex(int z) {
    zIndex_    = z;
    zIndexSet_ = true;
    return *this;
}

void applyModifier(QWidget* widget, const Modifier& modifier) {
    const int width = modifier.size_.width.toQtPixels(widget);
    const int height = modifier.size_.height.toQtPixels(widget);
    if (width > 0 && height > 0) {
        widget->setFixedSize(width, height);
    } else if (width > 0) {
        widget->setFixedWidth(width);
    } else if (height > 0) {
        widget->setFixedHeight(height);
    }

    if (modifier.fillMaxWidth_ || modifier.fillMaxHeight_) {
        QSizePolicy policy = widget->sizePolicy();
        if (modifier.fillMaxWidth_) {
            policy.setHorizontalPolicy(QSizePolicy::Expanding);
            widget->setProperty(kFillMaxWidthProperty, true);
        }
        if (modifier.fillMaxHeight_) {
            policy.setVerticalPolicy(QSizePolicy::Expanding);
            widget->setProperty(kFillMaxHeightProperty, true);
        }
        widget->setSizePolicy(policy);
    }

    // Store alignment as a property so Column/Row can apply it to their layouts.
    if (modifier.alignmentSet_) {
        widget->setProperty(kAlignmentProperty, static_cast<int>(modifier.alignment_));
    }

    if (modifier.weight_ > 0) {
        widget->setProperty(kWeightProperty, modifier.weight_);
    }

    if (modifier.onClick_) {
        widget->setProperty(kClickableProperty, true);
        widget->setCursor(Qt::PointingHandCursor);
        widget->installEventFilter(new ClickFilter(modifier.onClick_, widget));
    }

    if (modifier.alphaSet_) {
        widget->setProperty("rui_alpha", modifier.alpha_);
    }

    if (modifier.offsetSet_) {
        widget->setProperty(kOffsetXProperty, modifier.offsetX_.toQtPixels(widget));
        widget->setProperty(kOffsetYProperty, modifier.offsetY_.toQtPixels(widget));
    }

    if (modifier.zIndexSet_) {
        widget->setProperty(kZIndexProperty, modifier.zIndex_);
    }

    if (modifier.shadowSet_ && !modifier.clipSet_) {
        int elevation = modifier.shadowElevation_.toQtPixels(widget);
        auto* effect = new QGraphicsDropShadowEffect(widget);
        effect->setBlurRadius(elevation * 2);
        effect->setOffset(0, elevation);
        effect->setColor(QColor(0, 0, 0, 80));
        widget->setGraphicsEffect(effect);
    }

    const auto& m = modifier;
    const int paddingLeft = m.padding_.left.toQtPixels(widget);
    const int paddingTop = m.padding_.top.toQtPixels(widget);
    const int paddingRight = m.padding_.right.toQtPixels(widget);
    const int paddingBottom = m.padding_.bottom.toQtPixels(widget);
    const int fontSize = m.fontSize_.toQtPixels(widget);
    const int borderWidth = m.borderWidth_.toQtPixels(widget);
    const int cornerRadius = m.cornerRadius_.toQtPixels(widget);

    bool hasStyle = paddingLeft   > 0 || paddingTop    > 0 ||
                    paddingRight  > 0 || paddingBottom > 0 ||
                    m.foregroundSet_ ||
                    m.backgroundSet_ ||
                    fontSize       > 0 ||
                    borderWidth    > 0 ||
                    cornerRadius   > 0;

    if (m.clipSet_ && cornerRadius > 0) {
        widget->setProperty(kClipProperty, true);
        widget->setProperty(kCornerRadiusProperty, cornerRadius);
        widget->installEventFilter(new RoundedClipFilter(cornerRadius, widget));
    }

    if (!hasStyle) {
        return;
    }

    const char* cls = widget->metaObject()->className();
    QString style;

    if (paddingLeft > 0 || paddingTop > 0 || paddingRight > 0 || paddingBottom > 0) {
        style += QString("padding: %1px %2px %3px %4px;")
                     .arg(paddingTop).arg(paddingRight)
                     .arg(paddingBottom).arg(paddingLeft);
    }

    if (m.foregroundSet_) {
        style += QString("color: %1;").arg(m.foreground_.toQColor().name(QColor::HexArgb));
    }

    if (m.backgroundSet_) {
        style += QString("background: %1;").arg(m.background_.toQColor().name(QColor::HexArgb));
    }

    if (fontSize > 0) {
        style += QString("font-size: %1px;").arg(fontSize);
    }

    if (borderWidth > 0) {
        style += QString("border: %1px solid %2;")
                     .arg(borderWidth)
                     .arg(m.borderColor_.toQColor().name(QColor::HexArgb));
    } else {
        style += "border: none;";
    }

    if (cornerRadius > 0) {
        style += QString("border-radius: %1px;").arg(cornerRadius);
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

int modifierWeight(const QWidget* widget) {
    QVariant v = widget->property(kWeightProperty);
    return v.isValid() ? v.toInt() : 0;
}

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

static void insertStretches(QBoxLayout* layout, Arrangement a, int itemCount) {
    if (itemCount <= 0) {
        return;
    }

    // Remove old stretch items first.
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
        addStretch(0);
        break;
    case Arrangement::End:
        layout->insertStretch(0, 0);
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
