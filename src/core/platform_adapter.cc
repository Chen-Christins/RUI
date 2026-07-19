// platform_adapter.cc
#include "platform_adapter.hpp"

#include <QJsonObject>
#include <QPixmap>
#include <QBuffer>
#include <QPushButton>
#include <QRect>
#include <QRegularExpression>

#include "ui/color.hpp"

#include "devtools/devtools_server.hpp"
#include "devtools/widget_inspector.hpp"
#include "devtools/state_registry.hpp"
#include "devtools/layout_overlay.hpp"

namespace rui {

// ── 公共接口 ──────────────────────────────────────────

std::unique_ptr<PlatformAdapter> PlatformAdapter::create() {
    return std::unique_ptr<PlatformAdapter>(new PlatformAdapter());
}

void PlatformAdapter::initialize(int argc, char* argv[]) {
    app_ = std::make_unique<QApplication>(argc, argv);
}

void PlatformAdapter::setWindowSize(int width, int height) {
    initWindow();
    window_->resize(width, height);
}

void PlatformAdapter::setWindowTitle(const std::string& title) {
    initWindow();
    window_->setWindowTitle(QString::fromStdString(title));
}

void PlatformAdapter::enableDevTools(quint16 port) {
    devToolsPort_ = port;
    devToolsEnabled_ = true;
}

void PlatformAdapter::run(std::function<void()> content) {
    initWindow();

    auto* layout = new QVBoxLayout(window_.get());
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    currentLayout_ = layout;
    if (content) {
        content();
    }
    currentLayout_ = nullptr;

    if (devToolsEnabled_) {
        startDevTools();
    }

    window_->show();
    app_->exec();
}

QVBoxLayout* PlatformAdapter::currentLayout() {
    return currentLayout_;
}

// ── 私有 ──────────────────────────────────────────────

void PlatformAdapter::initWindow() {
    if (!window_) {
        window_ = std::make_unique<QWidget>();
        window_->setWindowTitle("RUI App");
    }
}

void PlatformAdapter::startDevTools() {
    auto& server = DevToolsServer::instance();
    server.setRootWidget(window_.get());
    server.start(devToolsPort_);

    server.registerMethod("getComponentTree", [this](const QJsonObject&) {
        QJsonObject resp;
        resp["tree"] = WidgetInspector::buildComponentTree(window_.get());
        return resp;
    });

    server.registerMethod("getWidgetDetail", [](const QJsonObject& params) {
        QString ptr = params["ptr"].toString();
        auto* widget = findWidgetByPtr(ptr);
        QJsonObject resp;
        if (widget) {
            resp["properties"] = widgetDetailToJson(widget);
        } else {
            resp["error"] = "Widget not found";
        }
        return resp;
    });

    server.registerMethod("getAllStates", [](const QJsonObject&) {
        QJsonObject resp;
        resp["states"] = StateRegistry::Instance()->getAllStates();
        return resp;
    });

    server.registerMethod("highlightWidget", [this](const QJsonObject& params) {
        QString ptr = params["ptr"].toString();
        QString colorStr = params["color"].toString();
        Color color = colorStr.isEmpty() ? Color(0, 120, 212, 80) : Color::fromHex(colorStr.toStdString());

        auto* widget = findWidgetByPtr(ptr);
        if (widget) {
            LayoutOverlay::Instance()->installOn(window_.get());
            LayoutOverlay::Instance()->setEnabled(true);
            LayoutOverlay::Instance()->highlightWidget(widget, color);
        }
        return QJsonObject{};
    });

    server.registerMethod("toggleLayoutOverlay", [this](const QJsonObject& params) {
        bool enabled = params["enabled"].toBool();
        LayoutOverlay::Instance()->installOn(window_.get());
        LayoutOverlay::Instance()->setEnabled(enabled);
        return QJsonObject{};
    });

    server.registerMethod("getScreenshot", [this](const QJsonObject&) {
        QJsonObject resp;
        if (window_) {
            QPixmap pixmap = window_->grab();
            QByteArray bytes;
            QBuffer buffer(&bytes);
            buffer.open(QIODevice::WriteOnly);
            pixmap.save(&buffer, "PNG");
            resp["image"] = QString::fromUtf8(bytes.toBase64());
            resp["imgWidth"] = pixmap.width();
            resp["imgHeight"] = pixmap.height();
            resp["logWidth"] = window_->width();
            resp["logHeight"] = window_->height();
        }
        return resp;
    });

    StateRegistry::Instance()->setChangeCallback(
        [&server](const std::string& tag, const std::string& value) {
            QJsonObject msg;
            msg["event"] = "stateChange";
            QJsonObject data;
            data["tag"] = QString::fromStdString(tag);
            data["value"] = QString::fromStdString(value);
            msg["data"] = data;
            server.broadcast(msg);
        });
}

QWidget* PlatformAdapter::findWidgetByPtr(const QString& ptrStr) {
    bool ok;
    uintptr_t addr = ptrStr.mid(2).toULongLong(&ok, 16);
    if (!ok) {
        return nullptr;
    }

    for (auto* topWidget : QApplication::topLevelWidgets()) {
        auto* found = findWidgetByAddr(topWidget, addr);
        if (found) {
            return found;
        }
    }
    return nullptr;
}

QWidget* PlatformAdapter::findWidgetByAddr(QWidget* root, uintptr_t addr) {
    if (reinterpret_cast<uintptr_t>(root) == addr) {
        return root;
    }
    for (auto* child : root->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly)) {
        auto* found = findWidgetByAddr(child, addr);
        if (found) {
            return found;
        }
    }
    return nullptr;
}

static QJsonObject colorToJson(const QColor& c) {
    QJsonObject o;
    o["r"] = c.red();
    o["g"] = c.green();
    o["b"] = c.blue();
    o["a"] = c.alpha();
    return o;
}

QJsonObject PlatformAdapter::widgetDetailToJson(QWidget* widget) {
    QJsonObject props;

    props["type"] = QString::fromUtf8(widget->metaObject()->className());
    props["objectName"] = widget->objectName();

    QJsonObject geo;
    QRect r = widget->geometry();
    geo["x"] = r.x();
    geo["y"] = r.y();
    geo["width"]  = r.width();
    geo["height"] = r.height();
    props["geometry"] = geo;

    props["visible"] = widget->isVisible();
    props["enabled"] = widget->isEnabled();

    // 样式 —— 仅从显式设置的 stylesheet 提取，不读 palette 默认值
    QJsonObject style;
    QString ss = widget->styleSheet();

    if (!ss.isEmpty()) {
        static QRegularExpression colorRe("color:\\s*([^;]+);");
        static QRegularExpression bgRe("(?:background|background-color):\\s*([^;]+);");
        static QRegularExpression fsRe("font-size:\\s*(\\d+)px;");
        static QRegularExpression padRe("padding:\\s*(\\d+)px\\s+(\\d+)px\\s+(\\d+)px\\s+(\\d+)px;");

        auto match = colorRe.match(ss);
        if (match.hasMatch()) {
            QColor c(match.captured(1).trimmed());
            if (c.isValid()) {
                style["foreground"] = colorToJson(c);
            }
        }

        match = bgRe.match(ss);
        if (match.hasMatch()) {
            QColor c(match.captured(1).trimmed());
            if (c.isValid()) {
                style["background"] = colorToJson(c);
            }
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
        props["style"] = style;
    }
    if (widget->layout()) {
        QJsonObject layout;
        layout["type"]  = QString::fromUtf8(widget->layout()->metaObject()->className());
        layout["count"] = widget->layout()->count();
        props["layout"] = layout;
    }

    props["childCount"] = widget->children().size();

    return props;
}

} // namespace rui
