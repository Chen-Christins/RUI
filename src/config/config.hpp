// config.hpp
#pragma once

#include <QString>
#include <QXmlStreamReader>

#include "utils/noncopyable.hpp"
#include "utils/singleton.hpp"

namespace rui {

/**
 * @brief 全局配置（XML 驱动）
 *
 * 查找 config.xml 的策略（cwd + 回退链）：
 *   1. config.xml
 *   2. ../config.xml
 *   3. ../../config.xml
 *
 * 解析失败时使用内置默认值，保证框架不因缺少配置文件而崩溃。
 */
class Config : public Singleton<Config>, Noncopyable {
public:
    /**
     * @brief 从 cwd 回退链查找并解析 config.xml
     *
     * 一般在 PlatformAdapter::initialize() 之后尽早调用。
     * 可重复调用 —— 后续调用会先重置为默认值再重新解析。
     */
    void load();

    /// 基准工作路径（绝对路径）
    QString workpath() const;

    /// 日志目录（绝对路径）
    QString logPath() const;

    /// DevTools 前端页面路径（绝对路径）
    QString frontendPath() const;

    /// 基于 workpath 解析相对路径
    QString resolvePath(const QString& relative) const;

private:
    friend class Singleton<Config>;
    Config() = default;

    void applyDefaults();

    QString findConfigFile() const;
    void parseXml(QXmlStreamReader& xml);
    void parseDevtools(QXmlStreamReader& xml);

private:
    QString workpath_;            // 绝对路径（config.xml 所在目录，即项目根目录）
    QString logDirRelative_;      // 相对路径（来自 XML）
    QString frontendRelative_;    // 相对路径（来自 XML）
    QString logPath_;             // 绝对路径（运行时计算）
    QString frontendPath_;        // 绝对路径（运行时计算）
};

} // namespace rui
