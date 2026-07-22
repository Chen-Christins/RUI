// widget_inspector.cc
#include "widget_inspector.hpp"

#include <QApplication>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMetaObject>
#include <QRegularExpression>
#include <QRect>
#include <QVariant>

#include <sstream>

namespace rui {

static constexpr const char* kComponentTypeProperty = "rui_component_type";

static QString widgetQtType(QWidget* widget) {
    return QString::fromUtf8(widget->metaObject()->className());
}

static QString widgetComponentType(QWidget* widget) {
    QVariant componentType = widget->property(kComponentTypeProperty);
    if (componentType.isValid() && !componentType.toString().isEmpty()) {
        return componentType.toString();
    }
    return widgetQtType(widget).remove("rui::");
}

static QString colorToHex(const QColor& c) {
    return QString("#%1%2%3%4")
        .arg(c.red(),   2, 16, QChar('0'))
        .arg(c.green(), 2, 16, QChar('0'))
        .arg(c.blue(),  2, 16, QChar('0'))
        .arg(c.alpha(), 2, 16, QChar('0'));
}

QJsonObject WidgetInspector::buildComponentTree(QWidget* root) {
    if (!root) {
        return {};
    }
    return widgetToJson(root);
}

QJsonObject WidgetInspector::buildAllWindowsTree() {
    QJsonArray windows;
    for (QWidget* w : QApplication::topLevelWidgets()) {
        if (w && w->isWindow()) {
            windows.append(widgetToJson(w));
        }
    }

    QJsonObject root;
    root["ptr"] = "0x0";
    root["type"] = "App";
    root["name"] = "All Windows (" + QString::number(windows.size()) + ")";
    root["width"] = 0;
    root["height"] = 0;
    root["x"] = 0;
    root["y"] = 0;
    root["visible"] = true;
    root["children"] = windows;
    return root;
}

QJsonObject WidgetInspector::widgetToJson(QWidget* widget) {
    QJsonObject obj;

    // 指针 ID
    std::ostringstream oss;
    oss << "0x" << std::hex << reinterpret_cast<uintptr_t>(widget);
    obj["ptr"] = QString::fromStdString(oss.str());

    // 类型 / 名称
    QString qtType = widgetQtType(widget);
    QString componentType = widgetComponentType(widget);
    obj["type"] = componentType;
    obj["qtType"] = qtType;
    QString name = widget->objectName();
    if (name.isEmpty()) {
        // 顶层窗口显示标题
        if (widget->isWindow() && !widget->windowTitle().isEmpty()) {
            name = widget->windowTitle();
        } else {
            name = componentType;
        }
    }
    obj["name"] = name;

    // 几何
    QRect geo = widget->geometry();
    obj["x"] = geo.x();
    obj["y"] = geo.y();
    obj["width"] = geo.width();
    obj["height"] = geo.height();
    obj["visible"] = widget->isVisible();

    // 文本
    if (auto* label = qobject_cast<QLabel*>(widget)) {
        obj["text"] = label->text();
    } else if (auto* button = qobject_cast<QPushButton*>(widget)) {
        obj["text"] = button->text();
    }

    // ── 样式信息（仅从显式设置的 stylesheet 中提取）──
    QJsonObject style;
    QString ss = widget->styleSheet();

    if (!ss.isEmpty()) {
        // 从 stylesheet 解析各属性
        static QRegularExpression colorRe("color:\\s*([^;]+);");
        static QRegularExpression bgRe("(?:background|background-color):\\s*([^;]+);");
        static QRegularExpression fsRe("font-size:\\s*(\\d+)px;");
        static QRegularExpression padRe("padding:\\s*(\\d+)px\\s+(\\d+)px\\s+(\\d+)px\\s+(\\d+)px;");

        auto match = colorRe.match(ss);
        if (match.hasMatch()) {
            style["foreground"] = match.captured(1).trimmed();
        }

        match = bgRe.match(ss);
        if (match.hasMatch()) {
            style["background"] = match.captured(1).trimmed();
        }

        match = fsRe.match(ss);
        if (match.hasMatch()) {
            style["fontSize"] = match.captured(1).toInt();
        }

        match = padRe.match(ss);
        if (match.hasMatch()) {
            QJsonObject pad;
            pad["top"]    = match.captured(1).toInt();
            pad["right"]  = match.captured(2).toInt();
            pad["bottom"] = match.captured(3).toInt();
            pad["left"]   = match.captured(4).toInt();
            style["padding"] = pad;
        }

        style["rawStyleSheet"] = ss;
    }

    if (!style.isEmpty()) {
        obj["style"] = style;
    }

    if (widget->layout()) {
        QJsonObject layoutObj;
        layoutObj["type"]  = QString::fromUtf8(widget->layout()->metaObject()->className());
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
