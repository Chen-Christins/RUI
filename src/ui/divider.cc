// divider.cc
#include "divider.hpp"

#include <QPainter>

#include "component_type.hpp"
#include "../utils/util.hpp"

namespace rui {

namespace {

class DividerWidget : public QWidget {
public:
    DividerWidget(Color color, int thickness)
        : QWidget(nullptr), color_(color) {
        utils::setComponentType(this, kUIDividerProperty);
        setFixedHeight(thickness);
    }

    QSize sizeHint() const override { return QSize(0, height()); }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.fillRect(rect(), color_.toQColor());
    }

private:
    Color color_;
};

} // namespace

Component Divider(
    Modifier modifier,
    Color color,
    Dimension thickness
) {
    int th = thickness.toQtPixels(nullptr);
    auto widget = std::make_unique<DividerWidget>(color, th);
    applyModifier(widget.get(), modifier);
    return widget;
}

} // namespace rui
