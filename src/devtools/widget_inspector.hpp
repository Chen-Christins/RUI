// widget_inspector.hpp
#pragma once

#include <QWidget>
#include <QJsonObject>
#include <QJsonArray>

namespace rui {

// Widget 树扫描器 — 遍历 QObject 层级并序列化为 JSON
class WidgetInspector {
public:
    // 从根 widget 开始，构建完整的组件树 JSON
    static QJsonObject buildComponentTree(QWidget* root);

    // 构建所有顶层窗口的组件树（包括主窗口和独立窗口）
    static QJsonObject buildAllWindowsTree();

private:
    static QJsonObject widgetToJson(QWidget* widget);
    static QJsonArray childrenToJson(QObject* parent);
};

} // namespace rui
