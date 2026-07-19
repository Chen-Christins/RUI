// platform_adapter.hpp
#pragma once

#include <QApplication>
#include <QJsonObject>
#include <QVBoxLayout>
#include <QWidget>

#include <functional>
#include <memory>
#include <string>

namespace rui {

class PlatformAdapter {
public:
    static std::unique_ptr<PlatformAdapter> create();

    void initialize(int argc, char* argv[]);
    void setWindowSize(int width, int height);
    void setWindowTitle(const std::string& title);
    void enableDevTools(quint16 port = 9222);
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
    std::unique_ptr<QApplication> app_;
    std::unique_ptr<QWidget> window_;
    static inline QVBoxLayout* currentLayout_ = nullptr;
    bool devToolsEnabled_ = false;
    quint16 devToolsPort_ = 9222;
};

} // namespace rui
