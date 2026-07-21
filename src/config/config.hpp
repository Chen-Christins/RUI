// config.hpp
#pragma once

#include <QString>
#include <QXmlStreamReader>

#include "utils/noncopyable.hpp"
#include "utils/singleton.hpp"

namespace rui {

/**
 * @brief 全局配置（XML 驱动）
 */
class Config : public Singleton<Config>, Noncopyable {
public:
    void load();

    QString workpath() const;

    bool logEnabled() const;
    
    QString logPath() const;

    QString frontendPath() const;

    QString resolvePath(const QString& relative) const;

private:
    friend class Singleton<Config>;
    Config() = default;

    void applyDefaults();

    QString findConfigFile() const;
    void parseXml(QXmlStreamReader& xml);
    void parseDevtools(QXmlStreamReader& xml);

private:
    QString workpath_;
    bool logEnabled_ = true;
    QString logFileRelative_;
    QString frontendRelative_;
    QString logPath_;
    QString frontendPath_;
};

} // namespace rui
