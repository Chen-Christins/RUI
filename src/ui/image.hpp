// image.hpp
#pragma once

#include <QPixmap>
#include <QWidget>

#include <memory>
#include <string>

#include "modifier.hpp"

namespace rui {

class ImageWidget : public QWidget {
public:
    explicit ImageWidget(const std::string& path, QWidget* parent = nullptr);

    const QPixmap& pixmap() const { return pixmap_; }
    void setPixmap(const QPixmap& pixmap);

    QSize sizeHint() const override;

protected:
    void showEvent(QShowEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private:
    QPixmap pixmap_;
};

std::unique_ptr<ImageWidget> Image(
    const std::string& path,
    Modifier modifier = Modifier{}
);

} // namespace rui
