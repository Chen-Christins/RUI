// badge.cc
#include "badge.hpp"

#include <QLabel>

#include "box.hpp"
#include "component_type.hpp"
#include "../utils/util.hpp"

namespace rui {

Component Badge(
    std::shared_ptr<MutableState<std::string>> text,
    Modifier modifier,
    Color backgroundColor
) {
    auto label = std::make_unique<QLabel>();
    utils::setComponentType(label.get(), kUIBadgeProperty);
    label->setAlignment(Qt::AlignCenter);
    label->setText(QString::fromStdString(text->get()));
    label->setMinimumHeight(20);
    label->setMinimumWidth(20);
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label->setStyleSheet(
        QString("QLabel { background: %1; color: #FFFFFF; border-radius: 10px;"
                " padding: 0 6px; font-size: 11px; border: none; }")
            .arg(backgroundColor.toQColor().name(QColor::HexArgb))
    );

    text->observe([raw = label.get()](const std::string& v) {
        raw->setText(QString::fromStdString(v));
    });

    applyModifier(label.get(), modifier);
    return label;
}

Component BadgedBox(
    Component badge,
    Modifier modifier,
    Component content
) {
    auto widget = std::make_unique<BoxWidget>();
    utils::setComponentType(widget.get(), kUIBoxProperty);
    applyModifier(widget.get(), modifier);

    if (content) {
        addToBox(widget.get(), std::move(content));
    }
    if (badge) {
        badge->setProperty(kAlignmentProperty, static_cast<int>(Alignment::TopEnd));
        addToBox(widget.get(), std::move(badge));
    }

    applyZOrder(widget.get());
    return widget;
}

} // namespace rui
