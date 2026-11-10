// dialog.cc
#include "dialog.hpp"

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

#include "../utils/util.hpp"
#include "component_type.hpp"

namespace rui {

namespace {

class AlertDialogWidget : public QDialog {
public:
    AlertDialogWidget(
        std::function<void()> onDismissRequest,
        Component confirmButton,
        Component dismissButton,
        const std::string& title,
        const std::string& text)
        : QDialog(nullptr)
        , onDismissRequest_(std::move(onDismissRequest))
    {
        utils::setComponentType(this, kUIDialogProperty);

        setWindowTitle(QString::fromStdString(title));
        setModal(true);

        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(24, 24, 24, 16);
        layout->setSpacing(12);

        auto* titleLabel = new QLabel(QString::fromStdString(title), this);
        titleLabel->setStyleSheet("QLabel { font-size: 18px; font-weight: bold; color: #212121; }");
        layout->addWidget(titleLabel);

        if (!text.empty()) {
            auto* textLabel = new QLabel(QString::fromStdString(text), this);
            textLabel->setWordWrap(true);
            textLabel->setStyleSheet("QLabel { font-size: 14px; color: #616161; }");
            layout->addWidget(textLabel);
        }

        auto* buttonRow = new QHBoxLayout();
        buttonRow->setSpacing(8);
        buttonRow->addStretch(1);

        if (dismissButton) {
            dismissButton_ = qobject_cast<QPushButton*>(dismissButton.release());
            if (dismissButton_) {
                buttonRow->addWidget(dismissButton_);
                QObject::connect(dismissButton_, &QPushButton::clicked, this, [this]() {
                    done(Rejected);
                });
            }
        }
        if (confirmButton) {
            confirmButton_ = qobject_cast<QPushButton*>(confirmButton.release());
            if (confirmButton_) {
                buttonRow->addWidget(confirmButton_);
                QObject::connect(confirmButton_, &QPushButton::clicked, this, [this]() {
                    done(Accepted);
                });
            }
        }

        layout->addLayout(buttonRow);
    }

protected:
    void done(int result) override
    {
        QDialog::done(result);
        if (onDismissRequest_) {
            onDismissRequest_();
        }
    }

private:
    std::function<void()> onDismissRequest_;
    QPushButton* confirmButton_ = nullptr;
    QPushButton* dismissButton_ = nullptr;
};

} // namespace

Component AlertDialog(
    std::function<void()> onDismissRequest,
    Component confirmButton,
    Component dismissButton,
    const std::string& title,
    const std::string& text,
    Modifier modifier)
{
    auto dialog = std::make_unique<AlertDialogWidget>(std::move(onDismissRequest), std::move(confirmButton), std::move(dismissButton), title, text);
    applyModifier(dialog.get(), modifier);
    return dialog;
}

} // namespace rui
