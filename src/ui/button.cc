// button.cc
#include "button.hpp"

namespace rui {

Composable std::unique_ptr<QPushButton> Button(
    std::function<void()> onClick,
    std::unique_ptr<QLabel> content,
    Modifier modifier
) {
    auto button = std::make_unique<QPushButton>();

    QString initialText = content->text();
    button->setText(initialText);
    button->setFlat(true);
    button->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    QObject::connect(button.get(), &QPushButton::clicked, onClick);

    applyModifier(button.get(), modifier);

    return button;
}

Composable std::unique_ptr<QPushButton> Button(
    std::function<void()> onClick,
    std::shared_ptr<MutableState<std::string>> text,
    Modifier modifier
) {
    auto button = std::make_unique<QPushButton>();

    button->setText(QString::fromStdString(text->get()));
    button->setFlat(true);
    button->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    text->observe([rawBtn = button.get()](const std::string& newText) {
        rawBtn->setText(QString::fromStdString(newText));
    });

    QObject::connect(button.get(), &QPushButton::clicked, onClick);

    applyModifier(button.get(), modifier);

    return button;
}

} // namespace rui
