// widget_inspector.cc
#include "widget_inspector.hpp"

#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMetaObject>
#include <QRect>

#include <sstream>

namespace rui {

QJsonObject WidgetInspector::buildComponentTree(QWidget* root) {
    if (!root) return {};
    return widgetToJson(root);
}

QJsonObject WidgetInspector::widgetToJson(QWidget* widget) {
    QJsonObject obj;

    // 指针地址作为唯一 ID（用于高亮定位）
    std::ostringstream oss;
    oss << "0x" << std::hex << reinterpret_cast<uintptr_t>(widget);
    obj["ptr"] = QString::fromStdString(oss.str());

    // 类型
    const QMetaObject* meta = widget->metaObject();
    obj["type"] = QString::fromUtf8(meta->className());

    // 名称（优先用 objectName，否则用类型）
    QString name = widget->objectName();
    if (name.isEmpty()) {
        name = QString::fromUtf8(meta->className()).remove("rui::");
    }
    obj["name"] = name;

    // 几何信息
    QRect geo = widget->geometry();
    obj["x"] = geo.x();
    obj["y"] = geo.y();
    obj["width"] = geo.width();
    obj["height"] = geo.height();
    obj["visible"] = widget->isVisible();

    // 文本内容（QLabel / QPushButton 等）
    if (auto* label = qobject_cast<QLabel*>(widget)) {
        obj["text"] = label->text();
    } else if (auto* button = qobject_cast<QPushButton*>(widget)) {
        obj["text"] = button->text();
    }

    // 布局信息
    if (widget->layout()) {
        QJsonObject layoutObj;
        layoutObj["type"] = QString::fromUtf8(widget->layout()->metaObject()->className());
        layoutObj["count"] = widget->layout()->count();
        obj["layout"] = layoutObj;
    }

    // 子组件
    obj["children"] = childrenToJson(widget);

    return obj;
}

QJsonArray WidgetInspector::childrenToJson(QObject* parent) {
    QJsonArray arr;

    for (QObject* child : parent->children()) {
        auto* w = qobject_cast<QWidget*>(child);
        if (w && w->isWidgetType()) {
            arr.append(widgetToJson(w));
        }
    }

    return arr;
}

} // namespace rui
