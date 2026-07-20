// layout_overlay.cc
#include "layout_overlay.hpp"

#include <QPainter>
#include <QPen>

namespace rui {

// 覆盖层 widget：绘制半透明边框
class OverlayWidget : public QWidget {
public:
    OverlayWidget(QWidget* target, const Color& color, QWidget* parent)
        : QWidget(parent), target_(target), color_(color.toQColor())
    {
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setAttribute(Qt::WA_NoSystemBackground);
        updateGeometry();
        show();
    }

    void updateGeometry() {
        if (!target_) {
            return;
        }
        QPoint topLeft = target_->mapTo(parentWidget(), QPoint(0, 0));
        QSize size = target_->size();
        setGeometry(QRect(topLeft, size));
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        QColor fill = color_;
        fill.setAlpha(20);
        painter.fillRect(rect(), fill);

        QPen pen(color_);
        pen.setWidth(2);
        painter.setPen(pen);
        painter.drawRect(rect().adjusted(1, 1, -1, -1));
    }

private:
    QWidget* target_;
    QColor color_;
};

void LayoutOverlay::highlightWidget(QWidget* widget, const Color& color) {
    if (!enabled_ || !rootWindow_) {
        return;
    }

    clearHighlight(widget);

    auto* overlay = new OverlayWidget(widget, color, rootWindow_);
    overlays_[widget] = overlay;
}

void LayoutOverlay::clearHighlight(QWidget* widget) {
    if (overlays_.contains(widget)) {
        delete overlays_[widget];
        overlays_.remove(widget);
    }
}

void LayoutOverlay::clearAll() {
    for (auto* overlay : overlays_) {
        delete overlay;
    }
    overlays_.clear();
}

void LayoutOverlay::setEnabled(bool enabled) {
    enabled_ = enabled;
    if (!enabled) clearAll();
}

void LayoutOverlay::installOn(QWidget* rootWindow) {
    rootWindow_ = rootWindow;
}

} // namespace rui
