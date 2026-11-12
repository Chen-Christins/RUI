// image.hpp
#pragma once

#include <memory>
#include <optional>
#include <string>

#include "color.hpp"
#include "component.hpp"
#include "modifier.hpp"

class QNetworkAccessManager;
class QNetworkReply;
class QSvgRenderer;

namespace rui {

enum class ContentScale {
    Fit,
    Crop,
    FillBounds,
    Inside,
    FillWidth,
    FillHeight,
};

struct ColorFilter {
    Color color;
};

class ImageWidget : public QWidget {
public:
    explicit ImageWidget(
        const std::string& path,
        Alignment alignment = Alignment::Center,
        ContentScale contentScale = ContentScale::Fit,
        const std::string& contentDescription = "",
        const std::optional<ColorFilter>& colorFilter = std::nullopt,
        QWidget* parent = nullptr);
    ~ImageWidget();

    QSize sizeHint() const override;

protected:
    void showEvent(QShowEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private:
    void loadPath(const std::string& path);
    void loadSvg(const QString& path);
    void loadNetwork(const QUrl& url);
    void onNetworkDone(QNetworkReply* reply);
    QRectF computeTargetRect(const QSizeF& contentSize, const QRectF& bounds) const;
    QRectF computeSourceRect(const QSizeF& contentSize, const QSizeF& targetSize) const;

    enum class Type {
        None,
        Pixmap,
        Svg
    };
    Type type_ = Type::None;
    QPixmap pixmap_;
    QSvgRenderer* svgRenderer_ = nullptr;
    QNetworkAccessManager* net_ = nullptr;

    Alignment alignment_ = Alignment::Center;
    ContentScale contentScale_ = ContentScale::Fit;
    std::string contentDescription_;
    std::optional<ColorFilter> colorFilter_;
};

Component Image(
    const std::string& path,
    Modifier modifier = Modifier{},
    Alignment alignment = Alignment::Center,
    ContentScale contentScale = ContentScale::Fit,
    const std::string& contentDescription = "",
    const std::optional<ColorFilter>& colorFilter = std::nullopt);

} // namespace rui
