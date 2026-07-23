#include "text_field.hpp"

#include <QLineEdit>
#include <QLabel>
#include <QVBoxLayout>

#include "component_type.hpp"
#include "../utils/util.hpp"

namespace rui {

namespace {

class TextFieldWidget : public QWidget {
public:
    TextFieldWidget(
        std::shared_ptr<MutableState<std::string>> state,
        const std::string& placeholder,
        const std::string& label,
        bool enabled,
        bool readOnly,
        bool singleLine,
        bool outlined
    )
        : QWidget(nullptr)
        , state_(std::move(state))
    {
        setAttribute(Qt::WA_TranslucentBackground);
        utils::setComponentType(this, kUITextFieldProperty);

        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(2);

        if (!label.empty()) {
            auto* labelWidget = new QLabel(QString::fromStdString(label), this);
            labelWidget->setStyleSheet(
                "QLabel { font-size: 12px; color: #666; padding-left: 2px; }"
            );
            layout->addWidget(labelWidget);
        }

        editor_ = new QLineEdit(this);
        editor_->setText(QString::fromStdString(state_->get()));
        editor_->setPlaceholderText(QString::fromStdString(placeholder));
        editor_->setEnabled(enabled);
        editor_->setReadOnly(readOnly);
        editor_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        QString ss;
        if (outlined) {
            ss = "QLineEdit {"
                 "  border: 1px solid #ccc;"
                 "  border-radius: 4px;"
                 "  padding: 8px 12px;"
                 "  font-size: 14px;"
                 "  background: transparent;"
                 "}"
                 "QLineEdit:focus {"
                 "  border: 2px solid #1976D2;"
                 "  padding: 7px 11px;"
                 "}"
                 "QLineEdit:disabled {"
                 "  background: #f5f5f5;"
                 "  color: #999;"
                 "}";
        } else {
            ss = "QLineEdit {"
                 "  border: none;"
                 "  border-bottom: 1px solid #ccc;"
                 "  border-radius: 4px 4px 0 0;"
                 "  padding: 8px 12px;"
                 "  font-size: 14px;"
                 "  background: #f5f5f5;"
                 "}"
                 "QLineEdit:focus {"
                 "  border-bottom: 2px solid #1976D2;"
                 "  background: #e8e8e8;"
                 "}"
                 "QLineEdit:disabled {"
                 "  color: #999;"
                 "}"
                 "QLineEdit:read-only {"
                 "  color: #666;"
                 "}";
        }
        editor_->setStyleSheet(ss);
        layout->addWidget(editor_);

        // state -> widget
        state_->observe([this](const std::string& value) {
            QString current = editor_->text();
            QString next = QString::fromStdString(value);
            if (current != next) {
                editor_->setText(next);
            }
        });

        // widget -> state
        connect(editor_, &QLineEdit::textChanged, this, [this](const QString& text) {
            state_->set(text.toStdString());
        });
    }

private:
    std::shared_ptr<MutableState<std::string>> state_;
    QLineEdit* editor_ = nullptr;
};

} // namespace

Component TextField(
    std::shared_ptr<MutableState<std::string>> state,
    Modifier modifier,
    const std::string& placeholder,
    const std::string& label,
    bool enabled,
    bool readOnly,
    bool singleLine
) {
    auto widget = std::make_unique<TextFieldWidget>(
        std::move(state), placeholder, label, enabled, readOnly, singleLine, false
    );
    applyModifier(widget.get(), modifier);
    return widget;
}

Component OutlinedTextField(
    std::shared_ptr<MutableState<std::string>> state,
    Modifier modifier,
    const std::string& placeholder,
    const std::string& label,
    bool enabled,
    bool readOnly,
    bool singleLine
) {
    auto widget = std::make_unique<TextFieldWidget>(
        std::move(state), placeholder, label, enabled, readOnly, singleLine, true
    );
    applyModifier(widget.get(), modifier);
    return widget;
}

} // namespace rui
