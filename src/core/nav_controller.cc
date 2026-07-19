// nav_controller.cc
#include "nav_controller.hpp"

#include <QVBoxLayout>

namespace rui {

NavController::NavController()
    : container_(new QStackedWidget()) {
}

void NavController::push(std::unique_ptr<QWidget> screen) {
    if (!screen) {
        return;
    }

    int currentIdx = container_->currentIndex();

    // 清除当前索引之后的所有界面（处理 push → pop → push 场景）
    while (static_cast<int>(screens_.size()) > currentIdx + 1) {
        QWidget* w = screens_.back();
        container_->removeWidget(w);
        w->deleteLater();
        screens_.pop_back();
    }

    QWidget* raw = screen.release();
    container_->addWidget(raw);  // QStackedWidget 通过 reparent 获得所有权
    screens_.push_back(raw);
    container_->setCurrentIndex(static_cast<int>(screens_.size()) - 1);
}

void NavController::replace(std::unique_ptr<QWidget> screen) {
    if (!screen) {
        return;
    }

    int currentIdx = container_->currentIndex();

    if (currentIdx >= 0 && !screens_.empty()) {
        QWidget* oldWidget = screens_[currentIdx];
        container_->removeWidget(oldWidget);

        QWidget* newScreen = screen.release();
        container_->insertWidget(currentIdx, newScreen);

        oldWidget->deleteLater();
        screens_[currentIdx] = newScreen;
    } else {
        QWidget* raw = screen.release();
        container_->addWidget(raw);
        screens_.push_back(raw);
        currentIdx = static_cast<int>(screens_.size()) - 1;
    }

    container_->setCurrentIndex(currentIdx);
}

bool NavController::pop() {
    if (!canPop()) {
        return false;
    }

    int currentIdx = container_->currentIndex();
    QWidget* oldWidget = screens_[currentIdx];
    container_->removeWidget(oldWidget);

    oldWidget->deleteLater();
    screens_.erase(screens_.begin() + currentIdx);

    container_->setCurrentIndex(currentIdx - 1);
    return true;
}

bool NavController::canPop() const {
    return container_->currentIndex() > 0;
}

QStackedWidget* NavController::container() const {
    return container_;
}

void NavController::openWindow(std::unique_ptr<QWidget> content, const WindowConfig& config) {
    if (!content) {
        return;
    }

    auto* window = new QWidget(nullptr, Qt::Window);
    if (!config.title.empty()) {
        window->setWindowTitle(QString::fromStdString(config.title));
    }
    window->resize(config.width, config.height);
    window->setAttribute(Qt::WA_DeleteOnClose);

    if (config.minWidth > 0 || config.minHeight > 0) {
        std::pair<int, int> minSize = {
            config.minWidth > 0 ? config.minWidth : 0,
            config.minHeight > 0 ? config.minHeight : 0
        };
        window->setMinimumSize(minSize.first, minSize.second);
    }
    if (config.maxWidth > 0 || config.maxHeight > 0) {
        std::pair<int, int> maxSize = {
            config.maxWidth > 0 ? config.maxWidth : QWIDGETSIZE_MAX,
            config.maxHeight > 0 ? config.maxHeight : QWIDGETSIZE_MAX
        };
        window->setMaximumSize(maxSize.first, maxSize.second);
    }

    auto* layout = new QVBoxLayout(window);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(content.release());

    window->show();
}

} // namespace rui
