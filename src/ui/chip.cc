// chip.cc
#include "chip.hpp"

#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>

#include "../utils/util.hpp"
#include "component_type.hpp"

namespace rui {

namespace {

class ChipWidget : public QWidget {
public:
    ChipWidget(
        std::shared_ptr<MutableState<std::string>> label,
        std::shared_ptr<MutableState<bool>> selected,
        std::function<void()> onClick,
        std::function<void()> onClose,
        bool enabled)
        : QWidget(nullptr)
        , label_(std::move(label))
        , selected_(std::move(selected))
        , onClick_(std::move(onClick))
        , enabled_(enabled)
    {
        utils::setComponentType(this, kUIChipProperty);
        setCursor(Qt::PointingHandCursor);

        auto* layout = new QHBoxLayout(this);
        layout->setContentsMargins(12, 6, 12, 6);
        layout->setSpacing(4);

        labelWidget_ = new QLabel(QString::fromStdString(label_->get()), this);
        labelWidget_->setAttribute(Qt::WA_TransparentForMouseEvents);
        layout->addWidget(labelWidget_);

        if (onClose) {
            auto* closeBtn = new QPushButton("×", this);
            closeBtn->setCursor(Qt::PointingHandCursor);
            closeBtn->setFixedSize(18, 18);
            closeBtn->setStyleSheet(
                "QPushButton { border: none; border-radius: 9px; background: transparent; color: #757575; }"
                "QPushButton:hover { background: rgba(0,0,0,32); }");
            QObject::connect(closeBtn, &QPushButton::clicked, onClose);
            layout->addWidget(closeBtn);
        }

        label_->observe(this, [this](const std::string& v) {
            labelWidget_->setText(QString::fromStdString(v));
        });
        if (selected_) {
            // 注意：observer 在 MutableState::set 持锁时被回调，不能在此重新 get()，用传入的值。
            selected_->observe(this, [this](bool sel) {
                refreshStyle(sel);
            });
        }
        refreshStyle(selected_ ? selected_->get() : false);
    }

    QSize sizeHint() const override { return QSize(labelWidget_->sizeHint().width() + 28, labelWidget_->sizeHint().height() + 12); }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        const bool sel = selected_ && selected_->get();
        QColor bg;
        if (sel) {
            bg = QColor("#1976D2");
        } else if (enabled_) {
            bg = QColor(0, 0, 0, 20);
        } else {
            bg = QColor(0, 0, 0, 8);
        }

        p.setPen(Qt::NoPen);
        p.setBrush(bg);
        p.drawRoundedRect(rect(), height() / 2.0, height() / 2.0);
    }

    void mouseReleaseEvent(QMouseEvent* event) override
    {
        if (!enabled_ || event->button() != Qt::LeftButton) {
            return;
        }
        if (!rect().contains(event->pos())) {
            return;
        }
        if (onClick_) {
            onClick_();
        }
    }

private:
    void refreshStyle(bool sel)
    {
        QColor textColor;
        if (sel) {
            textColor = QColor("#FFFFFF");
        } else if (enabled_) {
            textColor = QColor("#424242");
        } else {
            textColor = QColor("#9E9E9E");
        }
        labelWidget_->setStyleSheet(QString("QLabel { color: %1; background: transparent; border: none; }").arg(textColor.name(QColor::HexArgb)));
        update();
    }

private:
    std::shared_ptr<MutableState<std::string>> label_;
    std::shared_ptr<MutableState<bool>> selected_;
    std::function<void()> onClick_;
    bool enabled_ = true;
    QLabel* labelWidget_ = nullptr;
};

} // namespace

Component Chip(
    std::shared_ptr<MutableState<std::string>> label,
    std::function<void()> onClick,
    Modifier modifier,
    std::function<void()> onClose,
    bool enabled)
{
    auto widget = std::make_unique<ChipWidget>(std::move(label), nullptr, std::move(onClick), std::move(onClose), enabled);
    applyModifier(widget.get(), modifier);
    return widget;
}

Component FilterChip(
    std::shared_ptr<MutableState<bool>> selected,
    std::shared_ptr<MutableState<std::string>> label,
    std::function<void()> onClick,
    Modifier modifier,
    bool enabled)
{
    auto widget = std::make_unique<ChipWidget>(std::move(label), std::move(selected), std::move(onClick), nullptr, enabled);
    applyModifier(widget.get(), modifier);
    return widget;
}

} // namespace rui
