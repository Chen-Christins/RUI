// platform_adapter.hpp
#pragma once

#include <QApplication>
#include <QJsonObject>
#include <QVBoxLayout>
#include <QWidget>

#include <functional>
#include <memory>
#include <string>

#include "window_config.hpp"

namespace rui {

class PlatformAdapter {
public:
    static std::unique_ptr<PlatformAdapter> create();

    void initialize(int argc, char* argv[]);
    void setWindowSize(Dimension width, Dimension height);
    void setWindowTitle(const std::string& title);
    void setWindowConfig(const WindowConfig& config);
    void enableDevTools(uint16_t port = 9222);
    void run(std::function<void()> content = nullptr);

    static QVBoxLayout* currentLayout();

private:
    PlatformAdapter() = default;

    void initWindow();
    void startDevTools();

    static QWidget* findWidgetByPtr(const QString& ptrStr);
    static QWidget* findWidgetByAddr(QWidget* root, uintptr_t addr);
    static QJsonObject widgetDetailToJson(QWidget* widget);

private:
    // 当前布局指针，用于在运行时获取当前布局
    static inline QVBoxLayout* currentLayout_ = nullptr;
    // QApplication 和 QWidget 的智能指针，确保在 PlatformAdapter 生命周期结束时自动释放资源
    std::unique_ptr<QApplication> app_;
    // 主窗口指针
    std::unique_ptr<QWidget> window_;
    // 是否启用开发者工具
    bool devToolsEnabled_ = false;
    // 开发者工具端口号，默认值为 9222
    uint16_t devToolsPort_ = 9222;
};

} // namespace rui
