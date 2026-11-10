// slider.cc
#include "slider.hpp"

#include <QMouseEvent>
#include <QPainter>
#include <algorithm>

#include "../utils/util.hpp"
#include "component_type.hpp"

namespace rui {

namespace {

constexpr int kTrackHeight = 4;
constexpr int kThumbRadius = 10;
constexpr int kWidgetHeight = 48;

class SliderWidget : public QWidget {
public:
    SliderWidget(
        std::shared_ptr<MutableState<float>> value,
        std::function<void(float)> onValueChange,
        float rangeStart,
        float rangeEnd,
        int steps,
        bool enabled)
        : QWidget(nullptr)
        , value_(std::move(value))
        , onValueChange_(std::move(onValueChange))
        , rangeStart_(rangeStart)
        , rangeEnd_(rangeEnd)
        , steps_(steps)
        , enabled_(enabled)
    {
        utils::setComponentType(this, kUISliderProperty);
        setFixedHeight(kWidgetHeight);
        setCursor(Qt::PointingHandCursor);
        value_->observe(this, [this](float) {
            update();
        });
    }

    QSize sizeHint() const override { return QSize(200, kWidgetHeight); }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        const int trackY = (height() - kTrackHeight) / 2;
        const int trackLeft = kThumbRadius;
        const int trackRight = width() - kThumbRadius;
        const int trackWidth = std::max(0, trackRight - trackLeft);

        // 轨道
        p.setPen(Qt::NoPen);
        p.setBrush(enabled_ ? QColor(0, 0, 0, 40) : QColor(0, 0, 0, 24));
        p.drawRoundedRect(trackLeft, trackY, trackWidth, kTrackHeight, kTrackHeight / 2, kTrackHeight / 2);

        // 已激活轨道
        const float ratio = normalizedValue();
        const int activeWidth = static_cast<int>(trackWidth * ratio);
        QColor accent = enabled_ ? QColor("#1976D2") : QColor(25, 118, 210, 120);
        p.setBrush(accent);
        if (activeWidth > 0) {
            p.drawRoundedRect(trackLeft, trackY, activeWidth, kTrackHeight, kTrackHeight / 2, kTrackHeight / 2);
        }

        // 拇指
        const int thumbX = trackLeft + activeWidth;
        QColor thumbColor = enabled_ ? QColor("#1976D2") : QColor("#9E9E9E");
        p.setBrush(thumbColor);
        p.drawEllipse(QPoint(thumbX, height() / 2), kThumbRadius, kThumbRadius);
    }

    void mousePressEvent(QMouseEvent* event) override
    {
        if (!enabled_ || event->button() != Qt::LeftButton) {
            return;
        }
        dragging_ = true;
        grabMouse();
        updateValueFromX(event->pos().x());
    }

    void mouseMoveEvent(QMouseEvent* event) override
    {
        if (!dragging_) {
            return;
        }
        updateValueFromX(event->pos().x());
    }

    void mouseReleaseEvent(QMouseEvent* event) override
    {
        if (event->button() != Qt::LeftButton || !dragging_) {
            return;
        }
        dragging_ = false;
        releaseMouse();
        updateValueFromX(event->pos().x());
    }

private:
    float normalizedValue() const
    {
        const float v = value_->get();
        if (rangeEnd_ == rangeStart_) {
            return 0.0f;
        }
        return std::clamp((v - rangeStart_) / (rangeEnd_ - rangeStart_), 0.0f, 1.0f);
    }

    void updateValueFromX(int x)
    {
        const int trackLeft = kThumbRadius;
        const int trackRight = width() - kThumbRadius;
        const int trackWidth = std::max(0, trackRight - trackLeft);

        float ratio = 0.0f;
        if (trackWidth > 0) {
            ratio = std::clamp(static_cast<float>(x - trackLeft) / static_cast<float>(trackWidth), 0.0f, 1.0f);
        }

        float v = rangeStart_ + ratio * (rangeEnd_ - rangeStart_);
        if (steps_ > 0) {
            float step = (rangeEnd_ - rangeStart_) / static_cast<float>(steps_);
            v = rangeStart_ + std::round((v - rangeStart_) / step) * step;
            v = std::clamp(v, rangeStart_, rangeEnd_);
        }

        value_->set(v);
        if (onValueChange_) {
            onValueChange_(v);
        }
        update();
    }

private:
    std::shared_ptr<MutableState<float>> value_;
    std::function<void(float)> onValueChange_;
    float rangeStart_ = 0.0f;
    float rangeEnd_ = 1.0f;
    int steps_ = 0;
    bool enabled_ = true;
    bool dragging_ = false;
};

} // namespace

Component Slider(
    std::shared_ptr<MutableState<float>> value,
    std::function<void(float)> onValueChange,
    Modifier modifier,
    float valueRangeStart,
    float valueRangeEnd,
    int steps,
    bool enabled)
{
    auto widget = std::make_unique<SliderWidget>(std::move(value), std::move(onValueChange), valueRangeStart, valueRangeEnd, steps, enabled);
    applyModifier(widget.get(), modifier);
    return widget;
}

} // namespace rui
