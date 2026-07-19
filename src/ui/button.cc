// button.cc
#include "button.hpp"

namespace rui {

Composable std::unique_ptr<QPushButton> Button(
    std::function<void()> onClick,
    std::unique_ptr<QLabel> content,
    Modifier modifier
) {
    auto button = std::make_unique<QPushButton>();

    // 从 QLabel 中提取初始文本
    QString initialText = content->text();
    button->setText(initialText);

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

    // 订阅状态变化，实时更新按钮文本
    text->observe([rawBtn = button.get()](const std::string& newText) {
        rawBtn->setText(QString::fromStdString(newText));
    });

    QObject::connect(button.get(), &QPushButton::clicked, onClick);

    applyModifier(button.get(), modifier);

    return button;
}

} // namespace rui
