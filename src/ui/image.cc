// image.cc
#include "image.hpp"

#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <QShowEvent>

#include "component_type.hpp"
#include "../utils/util.hpp"

namespace rui {

ImageWidget::ImageWidget(const std::string& path, QWidget* parent)
        : QWidget(parent) {
    pixmap_.load(QString::fromStdString(path));
    if (!pixmap_.isNull()) {
        setMinimumSize(pixmap_.size());
    }
}

void ImageWidget::setPixmap(const QPixmap& pixmap) {
    pixmap_ = pixmap;
    if (!pixmap_.isNull()) {
        setMinimumSize(pixmap_.size());
    }
    update();
}

QSize ImageWidget::sizeHint() const {
    return pixmap_.isNull() ? QSize(0, 0) : pixmap_.size();
}

void ImageWidget::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
    // 清除 RoundedClipFilter 设置的 aliased mask，用 paintEvent 的平滑裁剪代替
    if (property(kCornerRadiusProperty).isValid()) {
        clearMask();
    }
}

void ImageWidget::paintEvent(QPaintEvent* event) {
    QWidget::paintEvent(event);

    if (pixmap_.isNull()) {
        return;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    QVariant cv = property(kCornerRadiusProperty);
    if (cv.isValid()) {
        int r = cv.toInt();
        QPainterPath path;
        path.addRoundedRect(rect(), r, r);
        painter.setClipPath(path);
    }

    qreal dpr = devicePixelRatioF();
    auto scaled = pixmap_.scaled(
        size() * dpr,
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation
    );
    scaled.setDevicePixelRatio(dpr);

    QPoint offset = rect().center() - scaled.rect().center() / dpr;
    painter.drawPixmap(offset, scaled);
}

std::unique_ptr<ImageWidget> Image(
    const std::string& path,
    Modifier modifier
) {
    auto widget = std::make_unique<ImageWidget>(path);
    utils::setComponentType(widget.get(), kUIImageProperty);
    applyModifier(widget.get(), modifier);
    return widget;
}

} // namespace rui
