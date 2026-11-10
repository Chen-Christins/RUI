// card.cc
#include "card.hpp"

#include <QGraphicsDropShadowEffect>
#include <QPainter>
#include <QVBoxLayout>

#include "../utils/util.hpp"
#include "component_type.hpp"

namespace rui {

namespace {

class CardWidget : public QWidget {
public:
    CardWidget(Component content, int radius, float elevation)
        : QWidget(nullptr)
        , radius_(radius)
    {
        utils::setComponentType(this, kUICardProperty);
        setAttribute(Qt::WA_TranslucentBackground);

        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        if (content) {
            layout->addWidget(content.release());
        }

        if (elevation > 0.0f) {
            auto* effect = new QGraphicsDropShadowEffect(this);
            effect->setBlurRadius(elevation * 6.0f);
            effect->setOffset(0.0f, elevation * 2.0f);
            effect->setColor(QColor(0, 0, 0, 60));
            setGraphicsEffect(effect);
        }
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#FFFFFF"));
        p.drawRoundedRect(rect().adjusted(1, 1, -1, -1), radius_, radius_);
    }

private:
    int radius_ = 8;
};

} // namespace

Component Card(Component content, Modifier modifier, Dimension cornerRadius, float elevation)
{
    int radius = cornerRadius.toQtPixels(nullptr);
    auto widget = std::make_unique<CardWidget>(std::move(content), radius, elevation);
    applyModifier(widget.get(), modifier);
    return widget;
}

} // namespace rui
