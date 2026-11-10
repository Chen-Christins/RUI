// surface.cc
#include "surface.hpp"

#include <QPainter>
#include <QVBoxLayout>

#include "../utils/util.hpp"
#include "component_type.hpp"
#include "graphics_effect.hpp"
#include "theme.hpp"

namespace rui {

namespace {

class SurfaceWidget : public QWidget {
public:
    SurfaceWidget(Component content, Color color, int radius, float elevation)
        : QWidget(nullptr)
        , color_(color)
        , radius_(radius)
    {
        utils::setComponentType(this, kUISurfaceProperty);
        setAttribute(Qt::WA_TranslucentBackground);

        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        if (content) {
            layout->addWidget(content.release());
        }

        if (elevation > 0.0f) {
            auto* effect = new RuiGraphicsEffect();
            effect->setShadow(static_cast<int>(elevation * 6.0f), 0, elevation * 2.0f, QColor(0, 0, 0, 60));
            setGraphicsEffect(effect);
        }
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(Qt::NoPen);
        p.setBrush(color_.toQColor());
        p.drawRoundedRect(rect().adjusted(1, 1, -1, -1), radius_, radius_);
    }

private:
    Color color_;
    int radius_ = 8;
};

} // namespace

Component Surface(Component content, Modifier modifier, Color color, RoundedCornerShape shape, float elevation)
{
    if (color == Color::white()) {
        color = currentColorScheme().surface;
    }
    int radius = shape.radius.toQtPixels(nullptr);
    auto widget = std::make_unique<SurfaceWidget>(std::move(content), color, radius, elevation);
    applyModifier(widget.get(), modifier);
    return widget;
}

} // namespace rui
