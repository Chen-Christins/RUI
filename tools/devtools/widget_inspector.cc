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

#include "ui/dimension.hpp"
#include "ui/component_type.hpp"
#include "utils/util.hpp"

namespace rui {

QJsonObject WidgetInspector::buildComponentTree(QWidget* root) {
    if (!root) {
        return {};
    }
    return widgetToJson(root);
}

QJsonObject WidgetInspector::buildAllWindowsTree() {
    QJsonArray windows;
    for (QWidget* w : QApplication::topLevelWidgets()) {
        if (w && w->isWindow() && w->isVisible()) {
            windows.append(widgetToJson(w));
        }
    }

    QJsonObject root;
    root["ptr"] = "0x0";
    root["type"] = "App";
    root["name"] = "All Windows (" + QString::number(windows.size()) + ")";
    root["width"] = utils::dpDimension(0, nullptr);
    root["height"] = utils::dpDimension(0, nullptr);
    root["x"] = utils::dpDimension(0, nullptr);
    root["y"] = utils::dpDimension(0, nullptr);
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
    QString qtType = utils::widgetQtType(widget);
    QString componentType = utils::widgetComponentType(widget);
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
    obj["x"] = utils::dpDimension(geo.x(), widget);
    obj["y"] = utils::dpDimension(geo.y(), widget);
    obj["width"] = utils::dpDimension(geo.width(), widget);
    obj["height"] = utils::dpDimension(geo.height(), widget);
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
        static QRegularExpression borderRe("border:\\s*(\\d+)px\\s+solid\\s+([^;]+);");
        static QRegularExpression radiusRe("border-radius:\\s*(\\d+)px;");

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
            style["fontSize"] = utils::dpDimension(match.captured(1).toInt(), widget);
        }

        match = padRe.match(ss);
        if (match.hasMatch()) {
            QJsonObject pad;
            pad["top"]    = utils::dpDimension(match.captured(1).toInt(), widget);
            pad["right"]  = utils::dpDimension(match.captured(2).toInt(), widget);
            pad["bottom"] = utils::dpDimension(match.captured(3).toInt(), widget);
            pad["left"]   = utils::dpDimension(match.captured(4).toInt(), widget);
            style["padding"] = pad;
        }

        match = borderRe.match(ss);
        if (match.hasMatch()) {
            QJsonObject border;
            border["width"] = utils::dpDimension(match.captured(1).toInt(), widget);
            border["color"] = match.captured(2).trimmed();
            style["border"] = border;
        }

        match = radiusRe.match(ss);
        if (match.hasMatch()) {
            style["cornerRadius"] = utils::dpDimension(match.captured(1).toInt(), widget);
        }

        style["rawStyleSheet"] = ss;
    }

    if (!style.isEmpty()) {
        obj["style"] = style;
    }

    QJsonObject modifier;
    QVariant weight = widget->property(kWeightProperty);
    if (weight.isValid()) {
        modifier["weight"] = weight.toInt();
    }
    if (widget->property(kClickableProperty).toBool()) {
        modifier["clickable"] = true;
    }
    if (widget->property(kClipProperty).toBool()) {
        modifier["clip"] = true;
    }
    if (widget->property(kFillMaxWidthProperty).toBool()) {
        modifier["fillMaxWidth"] = true;
    }
    if (widget->property(kFillMaxHeightProperty).toBool()) {
        modifier["fillMaxHeight"] = true;
    }
    if (!modifier.isEmpty()) {
        obj["modifier"] = modifier;
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
        if (!w || !w->isWidgetType() || !w->isVisible()) continue;

        // 跳过 Qt 内部 widget（如 QScrollArea 的 viewport/scrollbar 容器）
        // 但递归展开其可见子节点
        if (w->objectName().startsWith("qt_")) {
            for (const auto& sub : childrenToJson(w)) {
                arr.append(sub);
            }
            continue;
        }

        arr.append(widgetToJson(w));
    }
    return arr;
}

} // namespace rui
