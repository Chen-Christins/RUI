// config.cc
#include "config.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>

#include <cstdlib>

#include "../utils/logger.hpp"

namespace rui {

static auto logger = LOG_NAME("App");

void ConfigManager::load() {
    logEnabled_ = true;
    logFileRelative_.clear();
    frontendRelative_.clear();
    workpath_.clear();

    QString filePath = findConfigFile();
    if (filePath.isEmpty()) {
        FATAL(logger) << "config.xml not found at: " << QCoreApplication::applicationDirPath() + "/conf/config.xml";
        std::exit(1);
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        FATAL(logger) << "Cannot open config.xml: " << filePath;
        std::exit(1);
    }

    QXmlStreamReader xml(&file);
    parseXml(xml);

    if (xml.hasError()) {
        FATAL(logger) << "config.xml parse error: " << xml.errorString();
        std::exit(1);
    }

    if (workpath_.isEmpty() || !QDir::isAbsolutePath(workpath_)) {
        FATAL(logger) << "<workpath> must be an absolute path in config.xml";
        std::exit(1);
    }

    INFO(logger) << "Loaded: " << filePath;
    applyDefaults();  // 填充 XML 中未出现的字段
}

QString ConfigManager::workpath() const {
    return workpath_;
}

bool ConfigManager::logEnabled() const {
    return logEnabled_;
}

QString ConfigManager::logPath() const {
    return logPath_;
}

QString ConfigManager::frontendPath() const {
    return frontendPath_;
}

QString ConfigManager::resolvePath(const QString& relative) const {
    if (QDir::isAbsolutePath(relative)) {
        return relative;
    }
    return QDir::cleanPath(workpath_ + QDir::separator() + relative);
}

void ConfigManager::applyDefaults() {
    if (logFileRelative_.isEmpty()) {
        logFileRelative_ = "logs/app.log";
    }
    if (frontendRelative_.isEmpty()) {
        frontendRelative_ = "frontend.html";
    }
    logPath_ = resolvePath(logFileRelative_);
    frontendPath_ = resolvePath(frontendRelative_);
}

QString ConfigManager::findConfigFile() const {
    QDir dir(QCoreApplication::applicationDirPath());
    QString path = dir.absoluteFilePath("conf/config.xml");
    if (QFile::exists(path)) {
        return path;
    }
    return {};
}

void ConfigManager::parseXml(QXmlStreamReader& xml) {
    while (!xml.atEnd() && !xml.hasError()) {
        QXmlStreamReader::TokenType token = xml.readNext();
        if (token != QXmlStreamReader::StartElement) {
            continue;
        }

        if (xml.name() == QStringLiteral("workpath")) {
            QString raw = xml.readElementText().trimmed();
            if (QDir::isAbsolutePath(raw)) {
                workpath_ = QDir::cleanPath(raw);
            } else if (!raw.isEmpty() && raw != ".") {
                WARN(logger) << "ignoring non-absolute workpath: " << raw;
            }
        } else if (xml.name() == QStringLiteral("logs")) {
            auto enabledAttr = xml.attributes().value("enabled");
            if (!enabledAttr.isEmpty()) {
                logEnabled_ = (enabledAttr == "true");
            }
            logFileRelative_ = xml.readElementText().trimmed();
        } else if (xml.name() == QStringLiteral("devtools")) {
            parseDevtools(xml);
        }
    }
}

void ConfigManager::parseDevtools(QXmlStreamReader& xml) {
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
