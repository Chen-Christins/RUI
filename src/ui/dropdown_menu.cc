// dropdown_menu.cc
#include "dropdown_menu.hpp"

#include <QPushButton>
#include <QVBoxLayout>

#include "component_type.hpp"
#include "../utils/util.hpp"

namespace rui {

namespace {

class DropdownMenuWidget : public QWidget {
public:
    DropdownMenuWidget(
        std::shared_ptr<MutableState<bool>> expanded,
        std::function<void()> onDismissRequest,
        std::vector<DropdownMenuItem> items
    )
        : QWidget(nullptr)
        , expanded_(std::move(expanded))
        , onDismissRequest_(std::move(onDismissRequest))
        , items_(std::move(items)) {
        utils::setComponentType(this, kUIDropdownMenuProperty);
        expanded_->observe([this](bool e) {
            if (e) {
                open();
            } else {
                close();
            }
        });
    }

    QSize sizeHint() const override { return QSize(0, 0); }

private:
    void open() {
        if (popup_) {
            return;
        }

        popup_ = new QWidget(nullptr, Qt::Popup | Qt::FramelessWindowHint);
        popup_->setAttribute(Qt::WA_DeleteOnClose);
        popup_->setStyleSheet(
            "QWidget { background: #FFFFFF; border: 1px solid #E0E0E0; border-radius: 4px; }"
        );

        auto* layout = new QVBoxLayout(popup_);
        layout->setContentsMargins(4, 4, 4, 4);
        layout->setSpacing(0);

        for (const auto& item : items_) {
            auto* btn = new QPushButton(QString::fromStdString(item.label), popup_);
            btn->setFlat(true);
            btn->setCursor(Qt::PointingHandCursor);
            btn->setStyleSheet(
                "QPushButton { text-align: left; padding: 8px 12px; border: none;"
                " background: transparent; color: #212121; }"
                "QPushButton:hover { background: rgba(0,0,0,24); }"
            );
            auto onClick = item.onClick;
            QObject::connect(btn, &QPushButton::clicked, this, [this, onClick]() {
                if (onClick) {
                    onClick();
                }
                closePopup();
            });
            layout->addWidget(btn);
        }

        QObject::connect(popup_, &QWidget::destroyed, this, [this]() {
            popup_ = nullptr;
            expanded_->set(false);
            if (onDismissRequest_) {
                onDismissRequest_();
            }
        });

        popup_->adjustSize();
        popup_->move(mapToGlobal(QPoint(0, height())));
        popup_->show();
    }

    void closePopup() {
        if (popup_) {
            popup_->close();
        }
    }

    void close() {
        closePopup();
    }

private:
    std::shared_ptr<MutableState<bool>> expanded_;
    std::function<void()> onDismissRequest_;
    std::vector<DropdownMenuItem> items_;
    QWidget* popup_ = nullptr;
};

} // namespace

Component DropdownMenu(
    std::shared_ptr<MutableState<bool>> expanded,
    std::function<void()> onDismissRequest,
    std::vector<DropdownMenuItem> items,
    Modifier modifier
) {
    auto widget = std::make_unique<DropdownMenuWidget>(
        std::move(expanded), std::move(onDismissRequest), std::move(items)
    );
    applyModifier(widget.get(), modifier);
    return widget;
}

} // namespace rui
