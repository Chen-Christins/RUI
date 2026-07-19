// platform_adapter.cc
#include "platform_adapter.hpp"

#include <QJsonObject>
#include <QPixmap>
#include <QBuffer>
#include <QColor>
#include <QRect>

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
        resp["states"] = StateRegistry::instance().getAllStates();
        return resp;
    });

    server.registerMethod("highlightWidget", [this](const QJsonObject& params) {
        QString ptr = params["ptr"].toString();
        QString colorStr = params["color"].toString();
        QColor color = colorStr.isEmpty() ? QColor(0, 120, 212, 80) : QColor(colorStr);

        auto* widget = findWidgetByPtr(ptr);
        if (widget) {
            LayoutOverlay::instance().installOn(window_.get());
            LayoutOverlay::instance().setEnabled(true);
            LayoutOverlay::instance().highlightWidget(widget, color);
        }
        return QJsonObject{};
    });

    server.registerMethod("toggleLayoutOverlay", [this](const QJsonObject& params) {
        bool enabled = params["enabled"].toBool();
        LayoutOverlay::instance().installOn(window_.get());
        LayoutOverlay::instance().setEnabled(enabled);
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

    StateRegistry::instance().setChangeCallback(
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
    if (!ok) return nullptr;

    for (auto* topWidget : QApplication::topLevelWidgets()) {
        auto* found = findWidgetByAddr(topWidget, addr);
        if (found) return found;
    }
    return nullptr;
}

QWidget* PlatformAdapter::findWidgetByAddr(QWidget* root, uintptr_t addr) {
    if (reinterpret_cast<uintptr_t>(root) == addr) return root;
    for (auto* child : root->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly)) {
        auto* found = findWidgetByAddr(child, addr);
        if (found) return found;
    }
    return nullptr;
}

QJsonObject PlatformAdapter::widgetDetailToJson(QWidget* widget) {
    QJsonObject props;
    props["type"] = QString::fromUtf8(widget->metaObject()->className());
    props["objectName"] = widget->objectName();

    QRect geo = widget->geometry();
    props["x"] = QString::number(geo.x());
    props["y"] = QString::number(geo.y());
    props["width"] = QString::number(geo.width());
    props["height"] = QString::number(geo.height());

    props["visible"] = widget->isVisible() ? "true" : "false";
    props["enabled"] = widget->isEnabled() ? "true" : "false";

    auto size = widget->sizeHint();
    props["sizeHint"] = QString("%1 x %2").arg(size.width()).arg(size.height());

    props["children"] = QString::number(widget->children().size());

    return props;
}

} // namespace rui
