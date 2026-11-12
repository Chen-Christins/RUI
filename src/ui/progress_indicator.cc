// progress_indicator.cc
#include "progress_indicator.hpp"

#include <QPainter>
#include <algorithm>

#include "../utils/util.hpp"
#include "component_type.hpp"
#include "theme.hpp"

namespace rui {

namespace {

float clamp01(float v)
{
    return std::clamp(v, 0.0f, 1.0f);
}

class CircularProgressWidget : public QWidget {
public:
    CircularProgressWidget(std::shared_ptr<MutableState<float>> progress, Color color, int strokeWidth)
        : QWidget(nullptr)
        , progress_(std::move(progress))
        , color_(color)
        , strokeWidth_(strokeWidth)
    {
        utils::setComponentType(this, kUICircularProgressProperty);
        setFixedSize(48, 48);
        progress_->observe(this, [this](float) {
            update();
        });
    }

    QSize sizeHint() const override { return QSize(48, 48); }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        const int inset = strokeWidth_ / 2 + 1;
        const QRectF rect(inset, inset, width() - inset * 2, height() - inset * 2);

        // 背景环
        p.setPen(QPen(QColor(0, 0, 0, 24), strokeWidth_, Qt::SolidLine, Qt::RoundCap));
        p.setBrush(Qt::NoBrush);
        p.drawArc(rect, 0, 360 * 16);

        // 进度环，从顶部顺时针
        const float ratio = clamp01(progress_->get());
        p.setPen(QPen(color_.toQColor(), strokeWidth_, Qt::SolidLine, Qt::RoundCap));
        p.drawArc(rect, 90 * 16, -static_cast<int>(ratio * 360 * 16));
    }

private:
    std::shared_ptr<MutableState<float>> progress_;
    Color color_;
    int strokeWidth_ = 4;
};

class LinearProgressWidget : public QWidget {
public:
    LinearProgressWidget(std::shared_ptr<MutableState<float>> progress, Color color, Color trackColor, int strokeWidth)
        : QWidget(nullptr)
        , progress_(std::move(progress))
        , color_(color)
        , trackColor_(trackColor)
        , strokeWidth_(strokeWidth)
    {
        utils::setComponentType(this, kUILinearProgressProperty);
        setFixedHeight(strokeWidth);
        progress_->observe(this, [this](float) {
            update();
        });
    }

    QSize sizeHint() const override { return QSize(200, strokeWidth_); }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        // 轨道
        p.setPen(Qt::NoPen);
        p.setBrush(trackColor_.toQColor());
        p.drawRoundedRect(rect(), strokeWidth_ / 2, strokeWidth_ / 2);

        // 进度
        const float ratio = clamp01(progress_->get());
        const int fillWidth = static_cast<int>(width() * ratio);
        if (fillWidth > 0) {
            p.setBrush(color_.toQColor());
            p.drawRoundedRect(QRect(0, 0, fillWidth, height()), strokeWidth_ / 2, strokeWidth_ / 2);
        }
    }

private:
    std::shared_ptr<MutableState<float>> progress_;
    Color color_;
    Color trackColor_;
    int strokeWidth_ = 4;
};

} // namespace

Component CircularProgressIndicator(std::shared_ptr<MutableState<float>> progress, Modifier modifier, Color color, Dimension strokeWidth)
{
    const auto scheme = currentColorScheme();
    color = resolveThemeColor(color, scheme);
    auto widget = std::make_unique<CircularProgressWidget>(std::move(progress), color, strokeWidth.toQtPixels(nullptr));
    applyModifier(widget.get(), modifier);
    return widget;
}

Component LinearProgressIndicator(
    std::shared_ptr<MutableState<float>> progress,
    Modifier modifier,
    Color color,
    Color trackColor,
    Dimension strokeWidth)
{
    const auto scheme = currentColorScheme();
    color = resolveThemeColor(color, scheme);
    trackColor = resolveThemeColor(trackColor, scheme).withAlpha(40);
    auto widget = std::make_unique<LinearProgressWidget>(std::move(progress), color, trackColor, strokeWidth.toQtPixels(nullptr));
    applyModifier(widget.get(), modifier);
    return widget;
}

} // namespace rui
