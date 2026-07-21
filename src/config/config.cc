// config.cc
#include "config/config.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>

#include "utils/logger.hpp"

namespace rui {

static auto logger = Logger::getLogger("Config");

void Config::load() {
    // 重置相对路径字段
    logDirRelative_.clear();
    frontendRelative_.clear();

    QString filePath = findConfigFile();
    if (filePath.isEmpty()) {
        WARN(logger) << "config.xml not found, using defaults";
        workpath_ = QDir::currentPath();
        applyDefaults();
        return;
    }

    // workpath 默认就是 config.xml 所在目录（项目根目录）
    workpath_ = QFileInfo(filePath).absolutePath();

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        ERROR(logger) << "Cannot open " << filePath << ", using defaults";
        applyDefaults();
        return;
    }

    QXmlStreamReader xml(&file);
    parseXml(xml);

    if (xml.hasError()) {
        ERROR(logger) << "XML parse error: " << xml.errorString()
                      << ", using defaults";
        workpath_ = QDir::currentPath();
        applyDefaults();
    }

    INFO(logger) << "Loaded: " << filePath;
    applyDefaults();  // 填充 XML 中未出现的字段
}

QString Config::workpath() const {
    return workpath_;
}

QString Config::logPath() const {
    return logPath_;
}

QString Config::frontendPath() const {
    return frontendPath_;
}

QString Config::resolvePath(const QString& relative) const {
    if (QDir::isAbsolutePath(relative)) {
        return relative;
    }
    return QDir::cleanPath(workpath_ + QDir::separator() + relative);
}

void Config::applyDefaults() {
    if (logDirRelative_.isEmpty()) {
        logDirRelative_ = "logs/";
    }
    if (frontendRelative_.isEmpty()) {
        frontendRelative_ = "tools/devtools/frontend.html";
    }
    logPath_ = resolvePath(logDirRelative_);
    frontendPath_ = resolvePath(frontendRelative_);
}

QString Config::findConfigFile() const {
    static const char* kCandidates[] = {
        "config.xml",
        "../config.xml",
        "../../config.xml",
    };
    for (const auto* candidate : kCandidates) {
        if (QFile::exists(candidate)) {
            return QFileInfo(candidate).absoluteFilePath();
        }
    }
    return {};
}

void Config::parseXml(QXmlStreamReader& xml) {
    while (!xml.atEnd() && !xml.hasError()) {
        QXmlStreamReader::TokenType token = xml.readNext();
        if (token != QXmlStreamReader::StartElement) {
            continue;
        }

        if (xml.name() == QStringLiteral("workpath")) {
            QString raw = xml.readElementText().trimmed();
            // 仅在显式指定绝对路径时覆盖默认 workpath
            if (!raw.isEmpty() && raw != "." && QDir::isAbsolutePath(raw)) {
                workpath_ = QDir::cleanPath(raw);
            }
        } else if (xml.name() == QStringLiteral("logs")) {
            logDirRelative_ = xml.readElementText().trimmed();
        } else if (xml.name() == QStringLiteral("devtools")) {
            parseDevtools(xml);
        }
    }
}

void Config::parseDevtools(QXmlStreamReader& xml) {
    while (!xml.atEnd() && !xml.hasError()) {
        QXmlStreamReader::TokenType token = xml.readNext();

        if (token == QXmlStreamReader::EndElement &&
            xml.name() == QStringLiteral("devtools")) {
            break;
        }

        if (token == QXmlStreamReader::StartElement &&
            xml.name() == QStringLiteral("frontend")) {
            frontendRelative_ = xml.readElementText().trimmed();
        }
    }
}

} // namespace rui
