// graphics_effect.cc
#include "graphics_effect.hpp"

#include <QImage>
#include <QPainter>
#include <QPixmap>
#include <QTransform>

#include <algorithm>
#include <cmath>

namespace rui {

namespace {

// Separable box blur on an Alpha8 image.
QImage boxBlurAlpha(const QImage& src, int radius) {
    if (radius <= 0 || src.isNull()) {
        return src;
    }
    const int w = src.width();
    const int h = src.height();

    // Horizontal pass.
    QImage tmp(w, h, QImage::Format_Alpha8);
    for (int y = 0; y < h; ++y) {
        const uchar* in = src.constScanLine(y);
        uchar* out = tmp.scanLine(y);
        for (int x = 0; x < w; ++x) {
            int sum = 0;
            int cnt = 0;
            for (int k = -radius; k <= radius; ++k) {
                int xx = x + k;
                if (xx < 0) {
                    xx = 0;
                } else if (xx >= w) {
                    xx = w - 1;
                }
                sum += in[xx];
                ++cnt;
            }
            out[x] = static_cast<uchar>(sum / cnt);
        }
    }

    // Vertical pass.
    QImage res(w, h, QImage::Format_Alpha8);
    for (int x = 0; x < w; ++x) {
        for (int y = 0; y < h; ++y) {
            int sum = 0;
            int cnt = 0;
            for (int k = -radius; k <= radius; ++k) {
                int yy = y + k;
                if (yy < 0) {
                    yy = 0;
                } else if (yy >= h) {
                    yy = h - 1;
                }
                sum += tmp.constScanLine(yy)[x];
                ++cnt;
            }
            res.scanLine(y)[x] = static_cast<uchar>(sum / cnt);
        }
    }
    return res;
}

// Build a colored shadow image from the source's alpha channel.
QImage makeShadow(const QImage& src, const QColor& color, int radius) {
    QImage alpha(src.size(), QImage::Format_Alpha8);
    for (int y = 0; y < src.height(); ++y) {
        const QRgb* in = reinterpret_cast<const QRgb*>(src.constScanLine(y));
        uchar* out = alpha.scanLine(y);
        for (int x = 0; x < src.width(); ++x) {
            out[x] = static_cast<uchar>(qAlpha(in[x]));
        }
    }
    alpha = boxBlurAlpha(alpha, radius);

    QImage shadow(src.size(), QImage::Format_ARGB32);
    shadow.fill(Qt::transparent);
    const int colorAlpha = color.alpha();
    for (int y = 0; y < src.height(); ++y) {
        const uchar* a = alpha.constScanLine(y);
        QRgb* out = reinterpret_cast<QRgb*>(shadow.scanLine(y));
        for (int x = 0; x < src.width(); ++x) {
            out[x] = qRgba(color.red(), color.green(), color.blue(), (a[x] * colorAlpha) / 255);
        }
    }
    return shadow;
}

} // namespace

void RuiGraphicsEffect::setShadow(int blurRadius, qreal dx, qreal dy, const QColor& color) {
    hasShadow_ = true;
    blurRadius_ = blurRadius;
    shadowDx_ = dx;
    shadowDy_ = dy;
    shadowColor_ = color;
    update();
}

void RuiGraphicsEffect::setOpacity(qreal opacity) {
    opacity_ = opacity;
    update();
}

void RuiGraphicsEffect::setTransform(qreal rotationDegrees, qreal scaleX, qreal scaleY, qreal dx, qreal dy) {
    rotationDegrees_ = rotationDegrees;
    scaleX_ = scaleX;
    scaleY_ = scaleY;
    translateDx_ = dx;
    translateDy_ = dy;
    update();
}

QRectF RuiGraphicsEffect::boundingRectFor(const QRectF& sourceRect) const {
    qreal padX = 0.0;
    qreal padY = 0.0;

    if (hasShadow_) {
        padX = std::max(padX, blurRadius_ * 2.0 + std::fabs(shadowDx_));
        padY = std::max(padY, blurRadius_ * 2.0 + std::fabs(shadowDy_));
    }

    const bool transformed = rotationDegrees_ != 0.0 || scaleX_ != 1.0 || scaleY_ != 1.0 ||
                             translateDx_ != 0.0 || translateDy_ != 0.0;
    if (transformed) {
        const qreal rad = rotationDegrees_ * M_PI / 180.0;
        const qreal c = std::fabs(std::cos(rad));
        const qreal s = std::fabs(std::sin(rad));
        const qreal w = sourceRect.width();
        const qreal h = sourceRect.height();
        const qreal newW = w * std::fabs(scaleX_) * c + h * std::fabs(scaleY_) * s;
        const qreal newH = w * std::fabs(scaleX_) * s + h * std::fabs(scaleY_) * c;
        padX = std::max(padX, (newW - w) / 2.0 + std::fabs(translateDx_));
        padY = std::max(padY, (newH - h) / 2.0 + std::fabs(translateDy_));
    }

    if (padX <= 0.0 && padY <= 0.0) {
        return sourceRect;
    }
    return sourceRect.adjusted(-padX, -padY, padX, padY);
}

void RuiGraphicsEffect::draw(QPainter* painter) {
    QPoint offset;
    const QPixmap source = sourcePixmap(Qt::DeviceCoordinates, &offset);

    painter->save();
    painter->setWorldTransform(QTransform());
    painter->setOpacity(opacity_);

    // Transform around the source center.
    const QPointF center = offset + QPointF(source.width() / 2.0, source.height() / 2.0);
    painter->translate(center);
    if (rotationDegrees_ != 0.0) {
        painter->rotate(rotationDegrees_);
    }
    painter->scale(scaleX_, scaleY_);
    painter->translate(-center);
    painter->translate(translateDx_, translateDy_);

    if (hasShadow_) {
        const QImage shadow = makeShadow(source.toImage(), shadowColor_, blurRadius_);
        painter->drawImage(QPointF(offset) + QPointF(shadowDx_, shadowDy_), shadow);
    }
    painter->drawPixmap(offset, source);

    painter->restore();
}

} // namespace rui
