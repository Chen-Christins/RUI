// nav_controller.cc
#include "nav_controller.hpp"

#include "../ui/component_type.hpp"

#include <QVBoxLayout>

namespace rui {

NavController::NavController()
    : container_(new QStackedWidget()) {
    container_->setProperty(kComponentTypeProperty, kUINavHostProperty);
}

void NavController::push(std::unique_ptr<QWidget> screen) {
    if (!screen) {
        return;
    }

    int currentIdx = container_->currentIndex();

    // 清除当前索引之后的所有界面（处理 push → pop → push 场景）
    if (screens_.size() > (size_t)currentIdx + 1) {
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

    // 相同 key 的窗口已存在 → 拉到最前，不重复创建
    if (!config.key.empty()) {
        auto it = openWindows_.find(config.key);
        if (it != openWindows_.end() && it->second) {
            it->second->show();
            it->second->raise();
            it->second->activateWindow();
            return;
        }
    }

    auto* window = new QWidget(nullptr, Qt::Window);
    window->setProperty(kComponentTypeProperty, kUIWindowProperty);
    if (!config.title.empty()) {
        window->setWindowTitle(QString::fromStdString(config.title));
    }
    const int width = config.width.toQtPixels(window);
    const int height = config.height.toQtPixels(window);
    const int minWidth = config.minWidth.toQtPixels(window);
    const int minHeight = config.minHeight.toQtPixels(window);
    const int maxWidth = config.maxWidth.toQtPixels(window);
    const int maxHeight = config.maxHeight.toQtPixels(window);

    window->resize(width, height);
    window->setAttribute(Qt::WA_DeleteOnClose);

    if (minWidth > 0 || minHeight > 0) {
        std::pair minSize = {
            minWidth > 0 ? minWidth : 0,
            minHeight > 0 ? minHeight : 0
        };
        window->setMinimumSize(minSize.first, minSize.second);
    }
    if (maxWidth > 0 || maxHeight > 0) {
        std::pair maxSize = {
            maxWidth > 0 ? maxWidth : QWIDGETSIZE_MAX,
            maxHeight > 0 ? maxHeight : QWIDGETSIZE_MAX
        };
        window->setMaximumSize(maxSize.first, maxSize.second);
    }

    // 注册 key → 关闭时自动清除
    if (!config.key.empty()) {
        openWindows_[config.key] = window;
        QObject::connect(window, &QWidget::destroyed, [this, key = config.key]() {
            openWindows_.erase(key);
        });
    }

    auto layout = std::make_shared<QVBoxLayout>(window);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(content.release());

    window->show();
}

} // namespace rui
