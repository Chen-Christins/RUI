// graphics_effect.hpp
#pragma once

#include <QColor>
#include <QGraphicsEffect>

namespace rui {

// RuiGraphicsEffect —— 统一图形效果栈。
// 一个 widget 只能挂载一个 QGraphicsEffect，而 shadow 与 graphicsLayer（透明度/旋转/缩放/位移）
// 都需要占用 effect 位置。这里合并为一个自定义 effect，在 draw() 中按序合成。
class RuiGraphicsEffect : public QGraphicsEffect {
public:
    RuiGraphicsEffect() = default;

    void setShadow(int blurRadius, qreal dx, qreal dy, const QColor& color);
    void setOpacity(qreal opacity);
    void setTransform(qreal rotationDegrees, qreal scaleX, qreal scaleY, qreal dx, qreal dy);

protected:
    QRectF boundingRectFor(const QRectF& sourceRect) const override;
    void draw(QPainter* painter) override;

private:
    bool hasShadow_ = false;
    int blurRadius_ = 0;
    qreal shadowDx_ = 0.0;
    qreal shadowDy_ = 0.0;
    QColor shadowColor_;

    qreal opacity_ = 1.0;
    qreal rotationDegrees_ = 0.0;
    qreal scaleX_ = 1.0;
    qreal scaleY_ = 1.0;
    qreal translateDx_ = 0.0;
    qreal translateDy_ = 0.0;
};

} // namespace rui
