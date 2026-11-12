// tristate_checkbox.cc
#include "tristate_checkbox.hpp"

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>

#include "../utils/util.hpp"
#include "component_type.hpp"

namespace rui {

namespace {

class TriStateCheckboxWidget : public QWidget {
public:
    TriStateCheckboxWidget(std::shared_ptr<MutableState<TriState>> state, std::function<void(TriState)> onStateChange, bool enabled)
        : QWidget(nullptr)
        , state_(std::move(state))
        , onStateChange_(std::move(onStateChange))
        , enabled_(enabled)
    {
        utils::setComponentType(this, kUITriStateCheckboxProperty);
        setFixedSize(24, 24);
        setCursor(Qt::PointingHandCursor);
        state_->observe(this, [this](TriState) {
            update();
        });
    }

    QSize sizeHint() const override { return QSize(24, 24); }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        const TriState s = state_->get();
        const QRectF box(2.0f, 2.0f, 20.0f, 20.0f);
        const QColor fill = enabled_ ? QColor("#1976D2") : QColor(25, 118, 210, 120);

        if (s == TriState::Checked || s == TriState::Indeterminate) {
            p.setPen(Qt::NoPen);
            p.setBrush(fill);
            p.drawRoundedRect(box, 4, 4);

            QPen markPen(QColor("#FFFFFF"), 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
            p.setPen(markPen);
            p.setBrush(Qt::NoBrush);
            if (s == TriState::Checked) {
                QPainterPath path;
                path.moveTo(6.0f, 12.0f);
                path.lineTo(10.0f, 16.0f);
                path.lineTo(18.0f, 7.0f);
                p.drawPath(path);
            } else {
                p.drawLine(QPointF(7.0f, 12.0f), QPointF(17.0f, 12.0f));
            }
        } else {
            QColor border = enabled_ ? QColor("#757575") : QColor(0, 0, 0, 40);
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
        const TriState next = state_->get() == TriState::Checked ? TriState::Unchecked : TriState::Checked;
        state_->set(next);
        if (onStateChange_) {
            onStateChange_(next);
        }
        update();
    }

private:
    std::shared_ptr<MutableState<TriState>> state_;
    std::function<void(TriState)> onStateChange_;
    bool enabled_ = true;
};

} // namespace

Component TriStateCheckbox(
    std::shared_ptr<MutableState<TriState>> state,
    std::function<void(TriState)> onStateChange,
    bool enabled,
    Modifier modifier)
{
    auto widget = std::make_unique<TriStateCheckboxWidget>(std::move(state), std::move(onStateChange), enabled);
    applyModifier(widget.get(), modifier);
    return widget;
}

} // namespace rui
