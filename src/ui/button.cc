// button.cc
#include "button.hpp"

#include <QLabel>
#include <QPushButton>

#include "../utils/util.hpp"
#include "component_type.hpp"

namespace rui {

Component Button(
    std::function<void()> onClick,
    Component content,
    Modifier modifier
) {
    auto button = std::make_unique<QPushButton>();
    utils::setComponentType(button.get(), kUIButtonProperty);

    auto* label = qobject_cast<QLabel*>(content.get());
    if (label) {
        button->setText(label->text());
    }
    button->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    // 继承内部 Text 组件的 stylesheet（如 foreground、fontSize）
    QString innerStyle;
    if (label) {
        innerStyle = label->styleSheet();
    }
    if (!innerStyle.isEmpty()) {
        // 把 QLabel 的样式转成 QPushButton 的，作为初始样式
        innerStyle.replace("QLabel", "QPushButton");
        button->setStyleSheet(innerStyle);
    }

    QObject::connect(button.get(), &QPushButton::clicked, onClick);

    applyModifier(button.get(), modifier);

    return button;
}

Component Button(
    std::function<void()> onClick,
    std::shared_ptr<MutableState<std::string>> text,
    Modifier modifier
) {
    auto button = std::make_unique<QPushButton>();
    utils::setComponentType(button.get(), kUIButtonProperty);

    button->setText(QString::fromStdString(text->get()));
    button->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    text->observe([rawBtn = button.get()](const std::string& newText) {
        rawBtn->setText(QString::fromStdString(newText));
    });

    QObject::connect(button.get(), &QPushButton::clicked, onClick);

    applyModifier(button.get(), modifier);

    return button;
}

} // namespace rui
