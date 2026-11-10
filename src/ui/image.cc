// image.cc
#include "image.hpp"

#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPainter>
#include <QPainterPath>
#include <QShowEvent>
#include <QSvgRenderer>
#include <QUrl>
#include <algorithm>
#include <cmath>

#include "../utils/util.hpp"
#include "component_type.hpp"

namespace rui {

ImageWidget::ImageWidget(
    const std::string& path,
    Alignment alignment,
    ContentScale contentScale,
    const std::string& contentDescription,
    const std::optional<ColorFilter>& colorFilter,
    QWidget* parent)
    : QWidget(parent)
    , alignment_(alignment)
    , contentScale_(contentScale)
    , contentDescription_(contentDescription)
    , colorFilter_(colorFilter)
{
    this->setAttribute(Qt::WA_TranslucentBackground);
    loadPath(path);
}

ImageWidget::~ImageWidget()
{
    delete svgRenderer_;
}

void ImageWidget::loadPath(const std::string& path)
{
    QString qpath = QString::fromStdString(path);

    if (qpath.startsWith("http://") || qpath.startsWith("https://")) {
        loadNetwork(QUrl(qpath));
        return;
    }

    if (qpath.endsWith(".svg", Qt::CaseInsensitive)) {
        loadSvg(qpath);
        return;
    }

    pixmap_.load(qpath);
    if (!pixmap_.isNull()) {
        type_ = Type::Pixmap;
        this->setMinimumSize(pixmap_.size());
    }
}

void ImageWidget::loadSvg(const QString& path)
{
    auto* renderer = new QSvgRenderer(path);
    if (!renderer->isValid()) {
        delete renderer;
        return;
    }
    svgRenderer_ = renderer;
    type_ = Type::Svg;
    QSize sz = renderer->defaultSize();
    if (sz.isEmpty()) {
        sz = renderer->viewBoxF().size().toSize();
    }
    if (!sz.isEmpty()) {
        this->setMinimumSize(sz);
    }
}

void ImageWidget::loadNetwork(const QUrl& url)
{
    if (!net_) {
        net_ = new QNetworkAccessManager(this);
    }
    QNetworkReply* reply = net_->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onNetworkDone(reply);
    });
}

void ImageWidget::onNetworkDone(QNetworkReply* reply)
{
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        return;
    }

    QByteArray data = reply->readAll();

    if (reply->url().path().endsWith(".svg", Qt::CaseInsensitive) || data.startsWith("<svg") || data.startsWith("<?xml")) {
        auto* renderer = new QSvgRenderer(data);
        if (renderer->isValid()) {
            svgRenderer_ = renderer;
            type_ = Type::Svg;
            this->update();
            return;
        }
        delete renderer;
    }

    pixmap_.loadFromData(data);
    if (!pixmap_.isNull()) {
        type_ = Type::Pixmap;
        this->update();
    }
}

QSize ImageWidget::sizeHint() const
{
    switch (type_) {
    case Type::Pixmap:
        return pixmap_.isNull() ? QSize(0, 0) : pixmap_.size();
    case Type::Svg:
        if (svgRenderer_) {
            QSize sz = svgRenderer_->defaultSize();
            if (sz.isEmpty()) {
                sz = svgRenderer_->viewBoxF().size().toSize();
            }
            return sz;
        }
        return QSize(0, 0);
    default:
        return QSize(0, 0);
    }
}

static QPointF alignOffset(const QSizeF& content, const QRectF& bounds, Alignment a)
{
    qreal x = 0, y = 0;
    auto qt = toQtAlignment(a);
    if (qt & Qt::AlignRight) {
        x = bounds.width() - content.width();
    } else if (qt & Qt::AlignHCenter) {
        x = (bounds.width() - content.width()) / 2;
    }

    if (qt & Qt::AlignBottom) {
        y = bounds.height() - content.height();
    } else if (qt & Qt::AlignVCenter) {
        y = (bounds.height() - content.height()) / 2;
    }

    return bounds.topLeft() + QPointF(x, y);
}

QRectF ImageWidget::computeTargetRect(const QSizeF& contentSize, const QRectF& bounds) const
{
    if (contentSize.isEmpty() || bounds.isEmpty()) {
        return bounds;
    }

    qreal sx = bounds.width() / contentSize.width();
    qreal sy = bounds.height() / contentSize.height();

    switch (contentScale_) {
    case ContentScale::Fit: {
        qreal s = qMin(sx, sy);
        QSizeF sz = contentSize * s;
        return QRectF(alignOffset(sz, bounds, alignment_), sz);
    }
    case ContentScale::Crop:
        return bounds;
    case ContentScale::FillBounds:
        return bounds;
    case ContentScale::Inside: {
        if (contentSize.width() <= bounds.width() && contentSize.height() <= bounds.height()) {
            return QRectF(alignOffset(contentSize, bounds, alignment_), contentSize);
        }
        qreal s = qMin(sx, sy);
        QSizeF sz = contentSize * s;
        return QRectF(alignOffset(sz, bounds, alignment_), sz);
    }
    case ContentScale::FillWidth: {
        QSizeF sz(bounds.width(), contentSize.height() * sx);
        return QRectF(alignOffset(sz, bounds, alignment_), sz);
    }
    case ContentScale::FillHeight: {
        QSizeF sz(contentSize.width() * sy, bounds.height());
        return QRectF(alignOffset(sz, bounds, alignment_), sz);
    }
    }
    return bounds;
}

QRectF ImageWidget::computeSourceRect(const QSizeF& contentSize, const QSizeF& targetSize) const
{
    if (contentSize.isEmpty() || targetSize.isEmpty()) {
        return QRectF(QPointF(0, 0), contentSize);
    }

    qreal sx = targetSize.width() / contentSize.width();
    qreal sy = targetSize.height() / contentSize.height();

    switch (contentScale_) {
    case ContentScale::Crop: {
        qreal s = qMax(sx, sy);
        QSizeF srcSize(contentSize.width() * sx / s, contentSize.height() * sy / s);
        return QRectF(QPointF((contentSize.width() - srcSize.width()) / 2, (contentSize.height() - srcSize.height()) / 2), srcSize);
    }
    case ContentScale::FillWidth: {
        if (sy >= 1.0) {
            return QRectF(QPointF(0, 0), contentSize);
        }
        QSizeF srcSize(contentSize.width(), contentSize.height() * sy);
        return QRectF(QPointF(0, (contentSize.height() - srcSize.height()) / 2), srcSize);
    }
    case ContentScale::FillHeight: {
        if (sx >= 1.0) {
            return QRectF(QPointF(0, 0), contentSize);
        }
        QSizeF srcSize(contentSize.width() * sx, contentSize.height());
        return QRectF(QPointF((contentSize.width() - srcSize.width()) / 2, 0), srcSize);
    }
    default:
        return QRectF(QPointF(0, 0), contentSize);
    }
}

void ImageWidget::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    if (this->property(kCornerRadiusProperty).isValid()) {
        this->clearMask();
    }
}

void ImageWidget::paintEvent(QPaintEvent* event)
{
    QWidget::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    QVariant cv = this->property(kCornerRadiusProperty);
    if (cv.isValid()) {
        int r = cv.toInt();
        QPainterPath path;
        path.addRoundedRect(this->rect(), r, r);
        painter.setClipPath(path);
    }

    QVariant av = this->property("rui_alpha");
    if (av.isValid()) {
        painter.setOpacity(av.toFloat());
    }

    QRectF bounds = this->rect();

    switch (type_) {
    case Type::Pixmap: {
        QSizeF pixSize = pixmap_.size();
        QRectF dstRect = computeTargetRect(pixSize, bounds);
        QRectF srcRect = computeSourceRect(pixSize, dstRect.size());

        const qreal dpr = this->devicePixelRatioF();
        QSize physicalSize(
            std::max(1, static_cast<int>(std::round(dstRect.width() * dpr))), std::max(1, static_cast<int>(std::round(dstRect.height() * dpr))));
        QPixmap scaled = pixmap_.copy(srcRect.toAlignedRect()).scaled(physicalSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        scaled.setDevicePixelRatio(dpr);
        painter.drawPixmap(dstRect.topLeft(), scaled);

        if (colorFilter_) {
            painter.setCompositionMode(QPainter::CompositionMode_SourceAtop);
            painter.fillRect(dstRect, colorFilter_->color.toQColor());
        }
        break;
    }
    case Type::Svg:
        if (svgRenderer_) {
            QSizeF svgSize = svgRenderer_->defaultSize();
            if (svgSize.isEmpty()) {
                svgSize = svgRenderer_->viewBoxF().size();
            }
            if (!svgSize.isEmpty()) {
                QRectF dstRect = computeTargetRect(svgSize, bounds);
                svgRenderer_->render(&painter, dstRect);
            } else {
                svgRenderer_->render(&painter, bounds);
            }
        }
        break;
    default:
        break;
    }
}

Component Image(
    const std::string& path,
    Modifier modifier,
    Alignment alignment,
    ContentScale contentScale,
    const std::string& contentDescription,
    const std::optional<ColorFilter>& colorFilter)
{
    auto widget = std::make_unique<ImageWidget>(path, alignment, contentScale, contentDescription, colorFilter);
    utils::setComponentType(widget.get(), kUIImageProperty);
    applyModifier(widget.get(), modifier);
    return widget;
}

} // namespace rui
