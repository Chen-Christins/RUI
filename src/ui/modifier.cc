// modifier.cc
#include "modifier.hpp"

#include <QBoxLayout>
#include <QCursor>
#include <QMouseEvent>
#include <QPainterPath>
#include <QRegion>
#include <QSizePolicy>
#include <QString>
#include <QTimer>
#include <cmath>

#include "component_type.hpp"
#include "graphics_effect.hpp"

namespace rui {

namespace {

class ClickFilter : public QObject {
public:
    explicit ClickFilter(std::function<void()> onClick, QObject* parent = nullptr)
        : QObject(parent)
        , onClick_(std::move(onClick))
    {
    }

protected:
    bool eventFilter(QObject* watched, QEvent* event) override
    {
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
        : QObject(parent)
        , radius_(radius)
    {
    }

protected:
    bool eventFilter(QObject* watched, QEvent* event) override
    {
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

class AspectRatioFilter : public QObject {
public:
    explicit AspectRatioFilter(float ratio, QObject* parent = nullptr)
        : QObject(parent)
        , ratio_(ratio)
    {
    }

protected:
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        if (event->type() != QEvent::Resize) {
            return false;
        }
        auto* widget = qobject_cast<QWidget*>(watched);
        if (!widget || ratio_ <= 0.0f) {
            return false;
        }
        const int targetHeight = static_cast<int>(std::round(widget->width() / ratio_));
        if (widget->height() != targetHeight) {
            widget->setFixedHeight(targetHeight);
        }
        return false;
    }

private:
    float ratio_;
};

class SizeChangedFilter : public QObject {
public:
    explicit SizeChangedFilter(std::function<void(Size)> cb, QObject* parent = nullptr)
        : QObject(parent)
        , cb_(std::move(cb))
    {
    }

protected:
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        if (event->type() != QEvent::Resize) {
            return false;
        }
        if (auto* widget = qobject_cast<QWidget*>(watched)) {
            if (cb_) {
                cb_(Size{dp(widget->width()), dp(widget->height())});
            }
        }
        return false;
    }

private:
    std::function<void(Size)> cb_;
};

class GloballyPositionedFilter : public QObject {
public:
    explicit GloballyPositionedFilter(std::function<void(Offset)> cb, QObject* parent = nullptr)
        : QObject(parent)
        , cb_(std::move(cb))
    {
    }

protected:
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        if (event->type() != QEvent::Move && event->type() != QEvent::Show) {
            return false;
        }
        if (auto* widget = qobject_cast<QWidget*>(watched)) {
            if (cb_) {
                QPoint g = widget->mapToGlobal(QPoint(0, 0));
                cb_(Offset{dp(g.x()), dp(g.y())});
            }
        }
        return false;
    }

private:
    std::function<void(Offset)> cb_;
};

class FocusChangedFilter : public QObject {
public:
    explicit FocusChangedFilter(std::function<void(bool)> cb, QObject* parent = nullptr)
        : QObject(parent)
        , cb_(std::move(cb))
    {
    }

protected:
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        (void)watched;
        if (event->type() == QEvent::FocusIn) {
            if (cb_) {
                cb_(true);
            }
        } else if (event->type() == QEvent::FocusOut) {
            if (cb_) {
                cb_(false);
            }
        }
        return false;
    }

private:
    std::function<void(bool)> cb_;
};

class HoverFilter : public QObject {
public:
    explicit HoverFilter(std::function<void(bool)> cb, QObject* parent = nullptr)
        : QObject(parent)
        , cb_(std::move(cb))
    {
    }

protected:
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        (void)watched;
        if (event->type() == QEvent::Enter) {
            if (cb_) {
                cb_(true);
            }
        } else if (event->type() == QEvent::Leave) {
            if (cb_) {
                cb_(false);
            }
        }
        return false;
    }

private:
    std::function<void(bool)> cb_;
};

class CombinedClickFilter : public QObject {
public:
    CombinedClickFilter(std::function<void()> onLongClick, std::function<void()> onClick, QObject* parent = nullptr)
        : QObject(parent)
        , onLongClick_(std::move(onLongClick))
        , onClick_(std::move(onClick))
    {
        longPressTimer_ = new QTimer(this);
        longPressTimer_->setSingleShot(true);
        longPressTimer_->setInterval(500);
        connect(longPressTimer_, &QTimer::timeout, this, [this]() {
            longPressed_ = true;
            if (onLongClick_) {
                onLongClick_();
            }
        });
    }

protected:
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        auto* widget = qobject_cast<QWidget*>(watched);
        if (!widget) {
            return false;
        }

        if (event->type() == QEvent::MouseButtonPress) {
            auto* mouse = static_cast<QMouseEvent*>(event);
            if (mouse->button() == Qt::LeftButton) {
                longPressed_ = false;
                pressPos_ = mouse->pos();
                longPressTimer_->start();
            }
        } else if (event->type() == QEvent::MouseButtonRelease) {
            auto* mouse = static_cast<QMouseEvent*>(event);
            if (mouse->button() == Qt::LeftButton) {
                longPressTimer_->stop();
                if (!longPressed_ && onClick_ && widget->rect().contains(mouse->pos())) {
                    onClick_();
                }
            }
        } else if (event->type() == QEvent::MouseMove) {
            auto* mouse = static_cast<QMouseEvent*>(event);
            if ((mouse->pos() - pressPos_).manhattanLength() > 8) {
                longPressTimer_->stop();
            }
        }
        return false;
    }

private:
    std::function<void()> onLongClick_;
    std::function<void()> onClick_;
    QTimer* longPressTimer_ = nullptr;
    QPoint pressPos_;
    bool longPressed_ = false;
};

} // namespace

Modifier& Modifier::padding(Dimension all)
{
    return padding(all, all, all, all);
}

Modifier& Modifier::padding(Dimension horizontal, Dimension vertical)
{
    return padding(horizontal, vertical, horizontal, vertical);
}

Modifier& Modifier::padding(Dimension left, Dimension top, Dimension right, Dimension bottom)
{
    padding_ = {left, top, right, bottom};
    return *this;
}

Modifier& Modifier::size(Dimension width, Dimension height)
{
    size_ = {width, height};
    return *this;
}

Modifier& Modifier::width(Dimension width)
{
    size_.width = width;
    return *this;
}

Modifier& Modifier::height(Dimension height)
{
    size_.height = height;
    return *this;
}

Modifier& Modifier::fillMaxWidth()
{
    fillMaxWidth_ = true;
    return *this;
}

Modifier& Modifier::fillMaxHeight()
{
    fillMaxHeight_ = true;
    return *this;
}

Modifier& Modifier::fillMaxSize()
{
    return fillMaxWidth().fillMaxHeight();
}

Modifier& Modifier::foreground(Color color)
{
    foreground_ = color;
    foregroundSet_ = true;
    return *this;
}

Modifier& Modifier::background(Color color)
{
    background_ = color;
    backgroundSet_ = true;
    return *this;
}

Modifier& Modifier::background(Color color, RoundedCornerShape shape)
{
    background(color);
    cornerRadius_ = shape.radius;
    cornerRadiusSet_ = true;
    return *this;
}

Modifier& Modifier::fontSize(Dimension size)
{
    fontSize_ = size;
    return *this;
}

Modifier& Modifier::border(Color color, Dimension width)
{
    borderColor_ = color;
    borderWidth_ = width;
    return *this;
}

Modifier& Modifier::border(Color color, Dimension width, RoundedCornerShape shape)
{
    border(color, width);
    cornerRadius_ = shape.radius;
    cornerRadiusSet_ = true;
    return *this;
}

Modifier& Modifier::clip(RoundedCornerShape shape)
{
    cornerRadius_ = shape.radius;
    cornerRadiusSet_ = true;
    clipSet_ = true;
    return *this;
}

Modifier& Modifier::clickable(std::function<void()> onClick)
{
    onClick_ = std::move(onClick);
    return *this;
}

Modifier& Modifier::weight(int weight)
{
    weight_ = weight;
    return *this;
}

Modifier& Modifier::align(Alignment a)
{
    alignment_ = a;
    alignmentSet_ = true;
    return *this;
}

Modifier& Modifier::alpha(float opacity)
{
    alpha_ = opacity;
    alphaSet_ = true;
    return *this;
}

Modifier& Modifier::offset(Dimension x, Dimension y)
{
    offsetX_ = x;
    offsetY_ = y;
    offsetSet_ = true;
    return *this;
}

Modifier& Modifier::shadow(Dimension elevation, RoundedCornerShape shape)
{
    (void)shape;
    shadowElevation_ = elevation;
    shadowSet_ = true;
    return *this;
}

Modifier& Modifier::zIndex(int z)
{
    zIndex_ = z;
    zIndexSet_ = true;
    return *this;
}

Modifier& Modifier::requiredWidth(Dimension width)
{
    requiredWidth_ = width;
    return *this;
}

Modifier& Modifier::requiredHeight(Dimension height)
{
    requiredHeight_ = height;
    return *this;
}

Modifier& Modifier::requiredSize(Dimension width, Dimension height)
{
    requiredWidth_ = width;
    requiredHeight_ = height;
    return *this;
}

Modifier& Modifier::defaultMinSize(Dimension minWidth, Dimension minHeight)
{
    minWidth_ = minWidth;
    minHeight_ = minHeight;
    return *this;
}

Modifier& Modifier::aspectRatio(float ratio)
{
    aspectRatio_ = ratio;
    return *this;
}

Modifier& Modifier::onSizeChanged(std::function<void(rui::Size)> cb)
{
    onSizeChanged_ = std::move(cb);
    return *this;
}

Modifier& Modifier::onGloballyPositioned(std::function<void(Offset)> cb)
{
    onGloballyPositioned_ = std::move(cb);
    return *this;
}

Modifier& Modifier::focusable()
{
    focusable_ = true;
    return *this;
}

Modifier& Modifier::onFocusChanged(std::function<void(bool)> cb)
{
    onFocusChanged_ = std::move(cb);
    return *this;
}

Modifier& Modifier::combinedClickable(std::function<void()> onLongClick, std::function<void()> onClick)
{
    onLongClick_ = std::move(onLongClick);
    onClick_ = std::move(onClick);
    return *this;
}

Modifier& Modifier::hoverable(std::function<void(bool)> cb)
{
    onHoverChanged_ = std::move(cb);
    return *this;
}

Modifier& Modifier::graphicsLayer(std::function<void(GraphicsLayerScope&)> block)
{
    if (block) {
        block(graphicsLayer_);
    }
    graphicsLayerSet_ = true;
    return *this;
}

void applyModifier(QWidget* widget, const Modifier& modifier)
{
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

    if (modifier.onLongClick_) {
        widget->setProperty(kClickableProperty, true);
        widget->setCursor(Qt::PointingHandCursor);
        widget->installEventFilter(new CombinedClickFilter(modifier.onLongClick_, modifier.onClick_, widget));
    } else if (modifier.onClick_) {
        widget->setProperty(kClickableProperty, true);
        widget->setCursor(Qt::PointingHandCursor);
        widget->installEventFilter(new ClickFilter(modifier.onClick_, widget));
    }

    if (modifier.alphaSet_) {
        widget->setProperty("rui_alpha", modifier.alpha_);
    }

    if (modifier.backgroundSet_) {
        widget->setProperty(kBackgroundColorProperty, modifier.background_.toQColor());
    }

    if (modifier.offsetSet_) {
        widget->setProperty(kOffsetXProperty, modifier.offsetX_.toQtPixels(widget));
        widget->setProperty(kOffsetYProperty, modifier.offsetY_.toQtPixels(widget));
    }

    if (modifier.zIndexSet_) {
        widget->setProperty(kZIndexProperty, modifier.zIndex_);
    }

    if (modifier.requiredWidth_.value() >= 0 && modifier.requiredHeight_.value() >= 0) {
        widget->setMinimumSize(modifier.requiredWidth_.toQtPixels(widget), modifier.requiredHeight_.toQtPixels(widget));
        widget->setMaximumSize(modifier.requiredWidth_.toQtPixels(widget), modifier.requiredHeight_.toQtPixels(widget));
    } else if (modifier.requiredWidth_.value() >= 0) {
        widget->setMinimumWidth(modifier.requiredWidth_.toQtPixels(widget));
        widget->setMaximumWidth(modifier.requiredWidth_.toQtPixels(widget));
    } else if (modifier.requiredHeight_.value() >= 0) {
        widget->setMinimumHeight(modifier.requiredHeight_.toQtPixels(widget));
        widget->setMaximumHeight(modifier.requiredHeight_.toQtPixels(widget));
    }

    if (modifier.minWidth_.value() >= 0 || modifier.minHeight_.value() >= 0) {
        int minWidth = modifier.minWidth_.value() >= 0 ? modifier.minWidth_.toQtPixels(widget) : widget->minimumWidth();
        int minHeight = modifier.minHeight_.value() >= 0 ? modifier.minHeight_.toQtPixels(widget) : widget->minimumHeight();
        widget->setMinimumSize(minWidth, minHeight);
    }

    if (modifier.aspectRatio_ > 0.0f) {
        // 宽度已知时立即推导高度，避免依赖后续 resize 事件（否则初始高度为 0）。
        if (modifier.size_.width.value() > 0) {
            int fixedWidth = modifier.size_.width.toQtPixels(widget);
            widget->setFixedHeight(static_cast<int>(std::round(fixedWidth / modifier.aspectRatio_)));
        }
        widget->installEventFilter(new AspectRatioFilter(modifier.aspectRatio_, widget));
    }

    if (modifier.onSizeChanged_) {
        widget->installEventFilter(new SizeChangedFilter(modifier.onSizeChanged_, widget));
    }

    if (modifier.onGloballyPositioned_) {
        widget->installEventFilter(new GloballyPositionedFilter(modifier.onGloballyPositioned_, widget));
    }

    if (modifier.focusable_) {
        widget->setFocusPolicy(Qt::StrongFocus);
    }

    if (modifier.onFocusChanged_) {
        widget->setFocusPolicy(Qt::StrongFocus);
        widget->installEventFilter(new FocusChangedFilter(modifier.onFocusChanged_, widget));
    }

    if (modifier.onHoverChanged_) {
        widget->setAttribute(Qt::WA_Hover);
        widget->installEventFilter(new HoverFilter(modifier.onHoverChanged_, widget));
    }

    const bool needShadow = modifier.shadowSet_ && !modifier.clipSet_;
    if (needShadow || modifier.graphicsLayerSet_) {
        auto* effect = new RuiGraphicsEffect();
        widget->setGraphicsEffect(effect);
        if (needShadow) {
            int elevation = modifier.shadowElevation_.toQtPixels(widget);
            effect->setShadow(elevation * 2, 0, elevation, QColor(0, 0, 0, 80));
        }
        if (modifier.graphicsLayerSet_) {
            const auto& g = modifier.graphicsLayer_;
            effect->setOpacity(g.alpha);
            effect->setTransform(g.rotationZ, g.scaleX, g.scaleY, g.translationX.toQtPixels(widget), g.translationY.toQtPixels(widget));
        }
    }

    const auto& m = modifier;
    const int paddingLeft = m.padding_.left.toQtPixels(widget);
    const int paddingTop = m.padding_.top.toQtPixels(widget);
    const int paddingRight = m.padding_.right.toQtPixels(widget);
    const int paddingBottom = m.padding_.bottom.toQtPixels(widget);
    const int fontSize = m.fontSize_.toQtPixels(widget);
    const int borderWidth = m.borderWidth_.toQtPixels(widget);
    const int cornerRadius = m.cornerRadius_.toQtPixels(widget);

    bool hasStyle = paddingLeft > 0 || paddingTop > 0 || paddingRight > 0 || paddingBottom > 0 || m.foregroundSet_ || m.backgroundSet_ || fontSize > 0
        || borderWidth > 0 || cornerRadius > 0;

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
        style += QString("padding: %1px %2px %3px %4px;").arg(paddingTop).arg(paddingRight).arg(paddingBottom).arg(paddingLeft);
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
        style += QString("border: %1px solid %2;").arg(borderWidth).arg(m.borderColor_.toQColor().name(QColor::HexArgb));
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

int modifierWeight(const QWidget* widget)
{
    QVariant v = widget->property(kWeightProperty);
    return v.isValid() ? v.toInt() : 0;
}

Qt::Alignment toQtAlignment(Alignment a)
{
    switch (a) {
    case Alignment::TopStart:
        return Qt::AlignTop | Qt::AlignLeft;
    case Alignment::TopCenter:
        return Qt::AlignTop | Qt::AlignHCenter;
    case Alignment::TopEnd:
        return Qt::AlignTop | Qt::AlignRight;
    case Alignment::CenterStart:
        return Qt::AlignVCenter | Qt::AlignLeft;
    case Alignment::Center:
        return Qt::AlignCenter;
    case Alignment::CenterEnd:
        return Qt::AlignVCenter | Qt::AlignRight;
    case Alignment::BottomStart:
        return Qt::AlignBottom | Qt::AlignLeft;
    case Alignment::BottomCenter:
        return Qt::AlignBottom | Qt::AlignHCenter;
    case Alignment::BottomEnd:
        return Qt::AlignBottom | Qt::AlignRight;
    }
    return Qt::AlignTop | Qt::AlignLeft;
}

static void insertStretches(QBoxLayout* layout, Arrangement a, int itemCount)
{
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

void applyArrangement(QVBoxLayout* layout, Arrangement a, int itemCount)
{
    insertStretches(layout, a, itemCount);
}

void applyArrangement(QHBoxLayout* layout, Arrangement a, int itemCount)
{
    insertStretches(layout, a, itemCount);
}

} // namespace rui
