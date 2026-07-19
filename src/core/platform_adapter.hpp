// platform_adapter.hpp
#pragma once

#include <QApplication>
#include <QVBoxLayout>
#include <QWidget>

#include <functional>
#include <memory>

namespace rui {

class PlatformAdapter {
public:
    static std::unique_ptr<PlatformAdapter> create() {
        return std::unique_ptr<PlatformAdapter>(new PlatformAdapter());
    }

    void initialize(int argc, char* argv[]) {
        app_ = std::make_unique<QApplication>(argc, argv);
    }

    void setWindowSize(int width, int height) {
        if (!window_) {
            window_ = std::make_unique<QWidget>();
            window_->resize(width, height);
        }
    }

    void run(std::function<void()> content = nullptr) {
        // 创建主窗口
        if (!window_) {
            window_ = std::make_unique<QWidget>();
            window_->resize(800, 600);
        }

        auto* layout = new QVBoxLayout(window_.get());

        currentLayout_ = layout;
        if (content) {
            content();
        }
        currentLayout_ = nullptr;

        window_->show();
        app_->exec();
    }

    static QVBoxLayout* currentLayout() { return currentLayout_; }

private:
    PlatformAdapter() = default;

    std::unique_ptr<QApplication> app_;
    std::unique_ptr<QWidget> window_;
    static inline QVBoxLayout* currentLayout_ = nullptr;
};

} // namespace rui
