// platform_adapter.cc
#include "platform_adapter.hpp"

#include <QApplication>
#include <QCloseEvent>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonObject>
#include <QPixmap>
#include <QBuffer>
#include <QPushButton>
#include <QRect>
#include <QRegularExpression>
#include <QVariant>

#include <cstdlib>
#include <limits>

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

QString findConfigFile() {
    QDir dir(QCoreApplication::applicationDirPath());
    QString path = dir.absoluteFilePath("conf/config.xml");
    if (QFile::exists(path)) {
        return path;
    }
    return {};
}

QString normalizePathForCompare(const QString& path) {
    QString normalized = QDir::fromNativeSeparators(QDir::cleanPath(path));
    while (normalized.size() > 3 && normalized.endsWith('/')) {
        normalized.chop(1);
    }
    return normalized.toCaseFolded();
}

bool isPathUnderWorkpath(const QString& workpath, const QString& path) {
    const QString base = normalizePathForCompare(workpath);
    const QString target = normalizePathForCompare(path);
    return target == base || target.startsWith(base + "/");
}

QString resolveConfigPath(const QString& workpath, const QString& path) {
    QString resolved;
    if (QDir::isAbsolutePath(path)) {
        resolved = QDir::cleanPath(path);
    } else {
        resolved = QDir::cleanPath(workpath + QDir::separator() + path);
    }

    if (!isPathUnderWorkpath(workpath, resolved)) {
        return {};
    }
    return resolved;
}

std::size_t parseByteSize(const QString& value, std::size_t defaultValue) {
    const QString text = value.trimmed().toUpper();
    if (text.isEmpty()) {
        return defaultValue;
    }

    static QRegularExpression sizeRe(R"(^(\d+)\s*([KMG]?B?)?$)");
    const auto match = sizeRe.match(text);
    if (!match.hasMatch()) {
        WARN(rui::Logger::getLogger("App")) << "Invalid logs.max_size: " << value
                                            << ", fallback to " << defaultValue;
        return defaultValue;
    }

    bool ok = false;
    qulonglong size = match.captured(1).toULongLong(&ok);
    if (!ok) {
        WARN(rui::Logger::getLogger("App")) << "Invalid logs.max_size: " << value
                                            << ", fallback to " << defaultValue;
        return defaultValue;
    }

    const QString unit = match.captured(2);
    qulonglong multiplier = 1;
    if (unit.startsWith("K")) {
        multiplier = 1024;
    } else if (unit.startsWith("M")) {
        multiplier = 1024 * 1024;
    } else if (unit.startsWith("G")) {
        multiplier = 1024ull * 1024ull * 1024ull;
    }

    if (size > std::numeric_limits<std::size_t>::max() / multiplier) {
        WARN(rui::Logger::getLogger("App")) << "logs.max_size is too large: " << value
                                            << ", fallback to " << defaultValue;
        return defaultValue;
    }
    return static_cast<std::size_t>(size * multiplier);
}

} // namespace

namespace rui {

static Logger::ptr logger = Logger::getLogger("App");

static ConfigVar<std::string>::ptr gWorkpath =
    ConfigVar<std::string>::Lookup("workpath", {}, "Application work path");

static ConfigVar<bool>::ptr gLogEnabled =
    ConfigVar<bool>::Lookup("logs.enabled", true, "Enable file logging");

static ConfigVar<std::string>::ptr gLogLevel =
    ConfigVar<std::string>::Lookup("logs.level", Logger::defaultLevelName().toStdString(), "Log level");

static ConfigVar<std::string>::ptr gLogFile =
    ConfigVar<std::string>::Lookup("logs", "logs/app.log", "Log file path");

static ConfigVar<std::string>::ptr gLogMaxSize =
    ConfigVar<std::string>::Lookup("logs.max_size", "10MB", "Log rotation max file size");

static ConfigVar<int>::ptr gLogMaxFiles =
    ConfigVar<int>::Lookup("logs.max_files", 1, "Log rotation file count");

std::unique_ptr<PlatformAdapter> PlatformAdapter::create() {
    return std::unique_ptr<PlatformAdapter>(new PlatformAdapter());
}

void PlatformAdapter::initialize(int argc, char* argv[]) {
    app_ = std::make_unique<QApplication>(argc, argv);
    Logger::initConsole();

    QString configPath = findConfigFile();
    if (configPath.isEmpty()) {
        FATAL(logger) << "config.xml not found at: " << QCoreApplication::applicationDirPath() + "/conf/config.xml";
        std::exit(1);
    }

    QFile configFile(configPath);
    if (!configFile.open(QIODevice::ReadOnly)) {
        FATAL(logger) << "Cannot open config.xml: " << configPath;
        std::exit(1);
    }

    QString error;
    if (!ConfigMgr::Instance()->parse(&configFile, &error)) {
        FATAL(logger) << "config.xml parse error: " << error;
        std::exit(1);
    }

    const QString workpath = QString::fromStdString(gWorkpath->get());
    if (workpath.isEmpty() || !QDir::isAbsolutePath(workpath)) {
        FATAL(logger) << "<workpath> must be an absolute path in config.xml";
        std::exit(1);
    }

    Logger::setLevel(QString::fromStdString(gLogLevel->get()));
    INFO(logger) << "Loaded: " << configPath;
    if (gLogEnabled->get()) {
        const auto maxSize = parseByteSize(QString::fromStdString(gLogMaxSize->get()), 10 * 1024 * 1024);
        const auto maxFiles =
            gLogMaxFiles->get() > 0 ? static_cast<std::size_t>(gLogMaxFiles->get()) : 1;
        const QString logPath = resolveConfigPath(workpath, QString::fromStdString(gLogFile->get()));
        if (logPath.isEmpty()) {
            WARN(logger) << "Ignoring logs path outside workpath: " << gLogFile->get();
        } else {
            Logger::initFileAppender(logPath, maxSize, maxFiles);
        }
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
        window_->setProperty(kComponentTypeProperty, kUIWindowProperty);
        window_->setWindowTitle("RUI App");
    }
}

void PlatformAdapter::startDevTools() {
    DevToolsServerMgr::Instance()->setRootWidget(window_.get());
    DevToolsServerMgr::Instance()->start(devToolsPort_);

    DevToolsServerMgr::Instance()->registerMethod("getComponentTree", [](const QJsonObject&) {
        QJsonObject resp;
        resp["tree"] = WidgetInspector::buildAllWindowsTree();
        return resp;
    });

    DevToolsServerMgr::Instance()->registerMethod("getWidgetDetail", [](const QJsonObject& params) {
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

    DevToolsServerMgr::Instance()->registerMethod("getAllStates", [](const QJsonObject&) {
        QJsonObject resp;
        resp["states"] = StateRegistryMgr::Instance()->getAllStates();
        return resp;
    });

    DevToolsServerMgr::Instance()->registerMethod("getScreenshot", [this](const QJsonObject& params) {
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

    StateRegistryMgr::Instance()->setChangeCallback(
        [](const std::string& tag, const std::string& value) {
            QJsonObject msg;
            msg["event"] = "stateChange";
            QJsonObject data;
            data["tag"] = QString::fromStdString(tag);
            data["value"] = QString::fromStdString(value);
            msg["data"] = data;
            DevToolsServerMgr::Instance()->broadcast(msg);
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
