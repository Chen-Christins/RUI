// switch.cc
#include "switch.hpp"

#include <QMouseEvent>
#include <QPainter>

#include "component_type.hpp"
#include "../utils/util.hpp"

namespace rui {

namespace {

class SwitchWidget : public QWidget {
public:
    SwitchWidget(
        std::shared_ptr<MutableState<bool>> checked,
        std::function<void(bool)> onCheckedChange,
        bool enabled
    )
        : QWidget(nullptr)
        , checked_(std::move(checked))
        , onCheckedChange_(std::move(onCheckedChange))
        , enabled_(enabled) {
        utils::setComponentType(this, kUISwitchProperty);
        setFixedSize(52, 32);
        setCursor(Qt::PointingHandCursor);
        checked_->observe([this](bool) { update(); });
    }

    QSize sizeHint() const override { return QSize(52, 32); }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        const bool on = checked_->get();
        QColor trackColor = on ? QColor("#1976D2") : QColor(0, 0, 0, 60);
        if (!enabled_) {
            trackColor = on ? QColor(25, 118, 210, 120) : QColor(0, 0, 0, 40);
        }

        p.setPen(Qt::NoPen);
        p.setBrush(trackColor);
        p.drawRoundedRect(rect().adjusted(0, 0, -1, -1), 16, 16);

        const float tx = on ? width() - 28.0f : 4.0f;
        QColor thumbColor = enabled_ ? QColor("#FFFFFF") : QColor("#BDBDBD");
        p.setBrush(thumbColor);
        p.drawEllipse(QRectF(tx, 4.0f, 24.0f, 24.0f));
    }

    void mouseReleaseEvent(QMouseEvent* event) override {
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
};

} // namespace

Component Switch(
    std::shared_ptr<MutableState<bool>> checked,
    std::function<void(bool)> onCheckedChange,
    bool enabled,
    Modifier modifier
) {
    auto widget = std::make_unique<SwitchWidget>(
        std::move(checked), std::move(onCheckedChange), enabled
    );
    applyModifier(widget.get(), modifier);
    return widget;
}

} // namespace rui
