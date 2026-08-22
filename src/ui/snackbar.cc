// snackbar.cc
#include "snackbar.hpp"

#include <QLabel>
#include <QResizeEvent>
#include <QTimer>

#include "component_type.hpp"
#include "../utils/util.hpp"

namespace rui {

SnackbarHostState::SnackbarHostState()
    : message_(mutableStateOf(std::string("")))
    , generation_(mutableStateOf(0)) {}

void SnackbarHostState::showSnackbar(const std::string& message, SnackbarDuration duration) {
    duration_ = duration;
    message_->set(message);
    generation_->set(generation_->get() + 1);
}

namespace {

int durationMillis(SnackbarDuration d) {
    switch (d) {
    case SnackbarDuration::Short:
        return 4000;
    case SnackbarDuration::Long:
        return 10000;
    case SnackbarDuration::Indefinite:
        return 0;
    }
    return 4000;
}

class SnackbarHostWidget : public QWidget {
public:
    explicit SnackbarHostWidget(std::shared_ptr<SnackbarHostState> state, QWidget* parent = nullptr)
        : QWidget(parent), state_(std::move(state)) {
        utils::setComponentType(this, kUISnackbarHostProperty);
        setAttribute(Qt::WA_TranslucentBackground);

        label_ = new QLabel(this);
        label_->setAlignment(Qt::AlignCenter);
        label_->setWordWrap(true);
        label_->setStyleSheet(
            "QLabel {"
            "  background: #323232;"
            "  color: #ffffff;"
            "  border-radius: 4px;"
            "  padding: 12px 16px;"
            "  font-size: 14px;"
            "}"
        );
        label_->hide();

        timer_ = new QTimer(this);
        timer_->setSingleShot(true);
        QObject::connect(timer_, &QTimer::timeout, this, [this]() {
            label_->hide();
        });

        state_->generationState()->observe([this](int) {
            showCurrent();
        });
    }

protected:
    void resizeEvent(QResizeEvent* event) override {
        QWidget::resizeEvent(event);
        positionLabel();
    }

private:
    void showCurrent() {
        label_->setText(QString::fromStdString(state_->messageState()->get()));
        label_->adjustSize();

        int maxWidth = width() > 0 ? width() - 32 : 320;
        if (label_->width() > maxWidth) {
            label_->setFixedWidth(maxWidth);
        } else {
            label_->setMinimumWidth(0);
        }
        label_->show();
        label_->raise();

        int ms = durationMillis(state_->duration());
        if (ms > 0) {
            timer_->start(ms);
        } else {
            timer_->stop();
        }
        positionLabel();
    }

    void positionLabel() {
        if (!label_ || !label_->isVisible()) {
            return;
        }
        int margin = 16;
        int x = (width() - label_->width()) / 2;
        int y = height() - label_->height() - margin;
        label_->move(x, y);
    }

private:
    std::shared_ptr<SnackbarHostState> state_;
    QLabel* label_ = nullptr;
    QTimer* timer_ = nullptr;
};

} // namespace

Component SnackbarHost(
    std::shared_ptr<SnackbarHostState> hostState,
    Modifier modifier
) {
    auto widget = std::make_unique<SnackbarHostWidget>(std::move(hostState));
    applyModifier(widget.get(), modifier);
    return widget;
}

} // namespace rui
