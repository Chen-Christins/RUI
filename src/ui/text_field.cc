#include "text_field.hpp"

#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>

#include "../utils/util.hpp"
#include "component_type.hpp"
#include "theme.hpp"

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
        bool outlined)
        : QWidget(nullptr)
        , state_(std::move(state))
    {
        const auto scheme = currentColorScheme();
        const auto typography = currentTypography();
        setAttribute(Qt::WA_TranslucentBackground);
        utils::setComponentType(this, kUITextFieldProperty);

        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(2);

        if (!label.empty()) {
            auto* labelWidget = new QLabel(QString::fromStdString(label), this);
            labelWidget->setStyleSheet(QStringLiteral("QLabel { font-size: %1px; color: %2; padding-left: 2px; }")
                                           .arg(typography.labelFontSize)
                                           .arg(scheme.onSurface.toQColor().name(QColor::HexArgb)));
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
            ss = QStringLiteral("QLineEdit {"
                 "  border: 1px solid %1;"
                 "  border-radius: 4px;"
                 "  padding: 8px 12px;"
                 "  font-size: %2px;"
                 "  color: %3;"
                 "  background: %4;"
                 "}"
                 "QLineEdit:focus {"
                 "  border: 2px solid %5;"
                 "  padding: 7px 11px;"
                 "}"
                 "QLineEdit:disabled {"
                 "  background: %6;"
                 "  color: %7;"
                 "}")
                 .arg(scheme.onSurface.toQColor().name(QColor::HexArgb))
                 .arg(typography.bodyFontSize)
                 .arg(scheme.onSurface.toQColor().name(QColor::HexArgb))
                 .arg(scheme.surface.toQColor().name(QColor::HexArgb))
                 .arg(scheme.primary.toQColor().name(QColor::HexArgb))
                 .arg(scheme.background.toQColor().name(QColor::HexArgb))
                 .arg(scheme.onSurface.withAlpha(140).toQColor().name(QColor::HexArgb));
        } else {
            ss = QStringLiteral("QLineEdit {"
                 "  border: none;"
                 "  border-bottom: 1px solid %1;"
                 "  border-radius: 4px 4px 0 0;"
                 "  padding: 8px 12px;"
                 "  font-size: %2px;"
                 "  color: %3;"
                 "  background: %4;"
                 "}"
                 "QLineEdit:focus {"
                 "  border-bottom: 2px solid %5;"
                 "  background: %6;"
                 "}"
                 "QLineEdit:disabled {"
                 "  color: %7;"
                 "}"
                 "QLineEdit:read-only {"
                 "  color: %8;"
                 "}")
                 .arg(scheme.onSurface.toQColor().name(QColor::HexArgb))
                 .arg(typography.bodyFontSize)
                 .arg(scheme.onSurface.toQColor().name(QColor::HexArgb))
                 .arg(scheme.surface.toQColor().name(QColor::HexArgb))
                 .arg(scheme.primary.toQColor().name(QColor::HexArgb))
                 .arg(scheme.background.toQColor().name(QColor::HexArgb))
                 .arg(scheme.onSurface.withAlpha(140).toQColor().name(QColor::HexArgb))
                 .arg(scheme.onSurface.withAlpha(180).toQColor().name(QColor::HexArgb));
        }
        editor_->setStyleSheet(ss);
        layout->addWidget(editor_);

        // state -> widget
        state_->observe(this, [this](const std::string& value) {
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
    bool singleLine)
{
    auto widget = std::make_unique<TextFieldWidget>(std::move(state), placeholder, label, enabled, readOnly, singleLine, false);
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
    bool singleLine)
{
    auto widget = std::make_unique<TextFieldWidget>(std::move(state), placeholder, label, enabled, readOnly, singleLine, true);
    applyModifier(widget.get(), modifier);
    return widget;
}

} // namespace rui
