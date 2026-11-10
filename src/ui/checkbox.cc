// checkbox.cc
#include "checkbox.hpp"

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>

#include "../utils/util.hpp"
#include "component_type.hpp"
#include "theme.hpp"

namespace rui {

namespace {

class CheckboxWidget : public QWidget {
public:
    CheckboxWidget(std::shared_ptr<MutableState<bool>> checked, std::function<void(bool)> onCheckedChange, bool enabled)
        : QWidget(nullptr)
        , checked_(std::move(checked))
        , onCheckedChange_(std::move(onCheckedChange))
        , enabled_(enabled)
        , scheme_(currentColorScheme())
    {
        utils::setComponentType(this, kUICheckboxProperty);
        setFixedSize(24, 24);
        setCursor(Qt::PointingHandCursor);
        checked_->observe(this, [this](bool) {
            update();
        });
    }

    QSize sizeHint() const override { return QSize(24, 24); }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        const bool on = checked_->get();
        const QRectF box(2.0f, 2.0f, 20.0f, 20.0f);

        if (on) {
            QColor fill = scheme_.primary.toQColor();
            if (!enabled_) {
                fill.setAlpha(120);
            }
            p.setPen(Qt::NoPen);
            p.setBrush(fill);
            p.drawRoundedRect(box, 4, 4);

            QColor checkColor = scheme_.onPrimary.toQColor();
            QPen checkPen(checkColor, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
            p.setPen(checkPen);
            p.setBrush(Qt::NoBrush);
            QPainterPath path;
            path.moveTo(6.0f, 12.0f);
            path.lineTo(10.0f, 16.0f);
            path.lineTo(18.0f, 7.0f);
            p.drawPath(path);
        } else {
            QColor border = scheme_.onSurface.toQColor();
            border.setAlpha(enabled_ ? 180 : 40);
            p.setPen(QPen(border, 2));
            p.setBrush(Qt::NoBrush);
            p.drawRoundedRect(box, 4, 4);
        }
    }

    void mouseReleaseEvent(QMouseEvent* event) override
    {
        if (!enabled_ || event->button() != Qt::LeftButton) {
            return;
        }
        if (!rect().contains(event->pos())) {
            return;
        }
        const bool next = !checked_->get();
        checked_->set(next);
        if (onCheckedChange_) {
            onCheckedChange_(next);
        }
        update();
    }

private:
    std::shared_ptr<MutableState<bool>> checked_;
    std::function<void(bool)> onCheckedChange_;
    bool enabled_ = true;
    ColorScheme scheme_;
};

} // namespace

Component Checkbox(std::shared_ptr<MutableState<bool>> checked, std::function<void(bool)> onCheckedChange, bool enabled, Modifier modifier)
{
    auto widget = std::make_unique<CheckboxWidget>(std::move(checked), std::move(onCheckedChange), enabled);
    applyModifier(widget.get(), modifier);
    return widget;
}

} // namespace rui
