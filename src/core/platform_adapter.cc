// platform_adapter.cc
#include "platform_adapter.hpp"

#include <QApplication>
#include <QCloseEvent>
#include <QJsonObject>
#include <QPixmap>
#include <QBuffer>
#include <QPushButton>
#include <QRect>
#include <QRegularExpression>
#include <QVariant>

#include "../config/config.hpp"
#include "../ui/dimension.hpp"
#include "../ui/component_type.hpp"
#include "../utils/logger.hpp"
#include "../utils/util.hpp"

#include "devtools/devtools_server.hpp"
#include "devtools/widget_inspector.hpp"
#include "devtools/state_registry.hpp"

namespace {

// 主窗口：关闭时退出整个 app
class MainWindow : public QWidget {
public:
    using QWidget::QWidget;

protected:
    void closeEvent(QCloseEvent* event) override {
        QApplication::quit();
    }
};

} // namespace

namespace rui {

std::unique_ptr<PlatformAdapter> PlatformAdapter::create() {
    return std::unique_ptr<PlatformAdapter>(new PlatformAdapter());
}

void PlatformAdapter::initialize(int argc, char* argv[]) {
    app_ = std::make_unique<QApplication>(argc, argv);
    Logger::initConsole();
    Config::Instance()->load();
    if (Config::Instance()->logEnabled()) {
        Logger::initFileAppender(Config::Instance()->logPath());
    }
}

void PlatformAdapter::setWindowSize(Dimension width, Dimension height) {
    initWindow();
    window_->resize(width.toQtPixels(window_.get()), height.toQtPixels(window_.get()));
}

void PlatformAdapter::setWindowTitle(const std::string& title) {
    initWindow();
    window_->setWindowTitle(QString::fromStdString(title));
}

void PlatformAdapter::setWindowConfig(const WindowConfig& config) {
    initWindow();
    if (!config.title.empty()) {
        window_->setWindowTitle(QString::fromStdString(config.title));
    }
    const int width = config.width.toQtPixels(window_.get());
    const int height = config.height.toQtPixels(window_.get());
    const int minWidth = config.minWidth.toQtPixels(window_.get());
    const int minHeight = config.minHeight.toQtPixels(window_.get());
    const int maxWidth = config.maxWidth.toQtPixels(window_.get());
    const int maxHeight = config.maxHeight.toQtPixels(window_.get());

    window_->resize(width, height);
    if (minWidth > 0 || minHeight > 0) {
        std::pair<int, int> minSize = {
            minWidth > 0 ? minWidth : 0,
            minHeight > 0 ? minHeight : 0
        };
        window_->setMinimumSize(minSize.first, minSize.second);
    }
    if (maxWidth > 0 || maxHeight > 0) {
        std::pair<int, int> maxSize = {
            maxWidth > 0 ? maxWidth : QWIDGETSIZE_MAX,
            maxHeight > 0 ? maxHeight : QWIDGETSIZE_MAX
        };
        window_->setMaximumSize(maxSize.first, maxSize.second);
    }
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

void PlatformAdapter::initWindow() {
    if (!window_) {
        window_ = std::make_unique<MainWindow>();
        window_->setProperty(kComponentTypeProperty, "Window");
        window_->setWindowTitle("RUI App");
    }
}

void PlatformAdapter::startDevTools() {
    DevToolsServer::Instance()->setRootWidget(window_.get());
    DevToolsServer::Instance()->start(devToolsPort_);

    DevToolsServer::Instance()->registerMethod("getComponentTree", [](const QJsonObject&) {
        QJsonObject resp;
        resp["tree"] = WidgetInspector::buildAllWindowsTree();
        return resp;
    });

    DevToolsServer::Instance()->registerMethod("getWidgetDetail", [](const QJsonObject& params) {
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

    DevToolsServer::Instance()->registerMethod("getAllStates", [](const QJsonObject&) {
        QJsonObject resp;
        resp["states"] = StateRegistry::Instance()->getAllStates();
        return resp;
    });

    DevToolsServer::Instance()->registerMethod("getScreenshot", [this](const QJsonObject& params) {
        QJsonObject resp;

        // 根据传入的 ptr 找到对应窗口，默认使用主窗口
        QWidget* targetWindow = window_.get();
        QString ptrStr = params["ptr"].toString();
        if (!ptrStr.isEmpty()) {
            QWidget* w = findWidgetByPtr(ptrStr);
            if (w) {
                targetWindow = w->window();  // 找到所属顶层窗口
            }
        }

        if (targetWindow) {
            QPixmap pixmap = targetWindow->grab();
            QByteArray bytes;
            QBuffer buffer(&bytes);
            buffer.open(QIODevice::WriteOnly);
            pixmap.save(&buffer, "PNG");
            resp["image"] = QString::fromUtf8(bytes.toBase64());
            resp["imgWidth"] = utils::pxDimension(pixmap.width(), targetWindow);
            resp["imgHeight"] = utils::pxDimension(pixmap.height(), targetWindow);
            resp["logWidth"] = utils::dpDimension(targetWindow->width(), targetWindow);
            resp["logHeight"] = utils::dpDimension(targetWindow->height(), targetWindow);
        }
        return resp;
    });

    StateRegistry::Instance()->setChangeCallback(
        [](const std::string& tag, const std::string& value) {
            QJsonObject msg;
            msg["event"] = "stateChange";
            QJsonObject data;
            data["tag"] = QString::fromStdString(tag);
            data["value"] = QString::fromStdString(value);
            msg["data"] = data;
            DevToolsServer::Instance()->broadcast(msg);
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

QJsonObject PlatformAdapter::widgetDetailToJson(QWidget* widget) {
    QJsonObject props;

    props["type"] = utils::widgetComponentType(widget);
    props["qtType"] = utils::widgetQtType(widget);
    props["objectName"] = widget->objectName();

    QJsonObject geo;
    QRect r = widget->geometry();
    geo["x"] = utils::dpDimension(r.x(), widget);
    geo["y"] = utils::dpDimension(r.y(), widget);
    geo["width"]  = utils::dpDimension(r.width(), widget);
    geo["height"] = utils::dpDimension(r.height(), widget);
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
        static QRegularExpression borderRe("border:\\s*(\\d+)px\\s+solid\\s+([^;]+);");
        static QRegularExpression radiusRe("border-radius:\\s*(\\d+)px;");

        auto match = colorRe.match(ss);
        if (match.hasMatch()) {
            QColor c(match.captured(1).trimmed());
            if (c.isValid()) {
                style["foreground"] = utils::colorToJson(c);
            }
        }

        match = bgRe.match(ss);
        if (match.hasMatch()) {
            QColor c(match.captured(1).trimmed());
            if (c.isValid()) {
                style["background"] = utils::colorToJson(c);
            }
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
            QColor c(match.captured(2).trimmed());
            border["width"] = utils::dpDimension(match.captured(1).toInt(), widget);
            if (c.isValid()) {
                border["color"] = utils::colorToJson(c);
            }
            style["border"] = border;
        }

        match = radiusRe.match(ss);
        if (match.hasMatch()) {
            style["cornerRadius"] = utils::dpDimension(match.captured(1).toInt(), widget);
        }

        style["rawStyleSheet"] = ss;
    }

    if (!style.isEmpty()) {
        props["style"] = style;
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
        props["modifier"] = modifier;
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
