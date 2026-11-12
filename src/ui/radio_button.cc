// radio_button.cc
#include "radio_button.hpp"

#include <QMouseEvent>
#include <QPainter>

#include "../utils/util.hpp"
#include "component_type.hpp"
#include "theme.hpp"

namespace rui {

namespace {

class RadioButtonWidget : public QWidget {
public:
    RadioButtonWidget(std::shared_ptr<MutableState<bool>> selected, std::function<void(bool)> onSelectedChange, bool enabled)
        : QWidget(nullptr)
        , selected_(std::move(selected))
        , onSelectedChange_(std::move(onSelectedChange))
        , enabled_(enabled)
        , scheme_(currentColorScheme())
    {
        utils::setComponentType(this, kUIRadioButtonProperty);
        setFixedSize(24, 24);
        setCursor(Qt::PointingHandCursor);
        selected_->observe(this, [this](bool) {
            update();
        });
    }

    QSize sizeHint() const override { return QSize(24, 24); }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        const bool sel = selected_->get();
        const QRectF outer(2.0f, 2.0f, 20.0f, 20.0f);

        QColor accent = scheme_.primary.toQColor();
        QColor border = scheme_.onSurface.toQColor();
        accent.setAlpha(enabled_ ? 255 : 120);
        border.setAlpha(enabled_ ? 180 : 40);

        p.setPen(QPen(sel ? accent : border, 2));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(outer);

        if (sel) {
            p.setPen(Qt::NoPen);
            p.setBrush(accent);
            p.drawEllipse(outer.adjusted(5, 5, -5, -5));
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
        if (selected_->get()) {
            return;
        }
        selected_->set(true);
        if (onSelectedChange_) {
            onSelectedChange_(true);
        }
        update();
    }

private:
    std::shared_ptr<MutableState<bool>> selected_;
    std::function<void(bool)> onSelectedChange_;
    bool enabled_ = true;
    ColorScheme scheme_;
};

} // namespace

Component RadioButton(std::shared_ptr<MutableState<bool>> selected, std::function<void(bool)> onSelectedChange, bool enabled, Modifier modifier)
{
    auto widget = std::make_unique<RadioButtonWidget>(std::move(selected), std::move(onSelectedChange), enabled);
    applyModifier(widget.get(), modifier);
    return widget;
}

} // namespace rui
