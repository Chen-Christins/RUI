/**
 * @file logger.hpp
 * @brief Logger utility class for logging messages with different severity levels.
 * @author Christins (chen.christins@qq.com)
 * @date 2026-07-22
 * @copyright Apache 2.0
 */
#pragma once

#include <memory>
#include <sstream>
#include <vector>

#include <QString>

#include <spdlog/logger.h>
#include <spdlog/spdlog.h>

#include "noncopyable.hpp"

namespace rui {

#define TRACE(logger) ::rui::LogEntry((logger), spdlog::level::trace,    spdlog::source_loc{__FILE__, __LINE__, nullptr})
#define DEBUG(logger) ::rui::LogEntry((logger), spdlog::level::debug,    spdlog::source_loc{__FILE__, __LINE__, nullptr})
#define INFO(logger)  ::rui::LogEntry((logger), spdlog::level::info,     spdlog::source_loc{__FILE__, __LINE__, nullptr})
#define WARN(logger)  ::rui::LogEntry((logger), spdlog::level::warn,     spdlog::source_loc{__FILE__, __LINE__, nullptr})
#define ERROR(logger) ::rui::LogEntry((logger), spdlog::level::err,      spdlog::source_loc{__FILE__, __LINE__, nullptr})
#define FATAL(logger) ::rui::LogEntry((logger), spdlog::level::critical, spdlog::source_loc{__FILE__, __LINE__, nullptr})

#define LOG_NAME(name) ::rui::Logger::getLogger(name)

class Logger {
public:
    using ptr = std::shared_ptr<spdlog::logger>;

    static void initConsole();
    static void initFileAppender(const QString& filePath, std::size_t maxSizeBytes = 0, std::size_t maxFiles = 1);
    static void setLevel(const QString& level);
    static QString defaultLevelName();
    static std::shared_ptr<spdlog::logger> getLogger(const QString& name);

private:
    static const char* pattern();
    static std::mutex& mutex();
    static std::vector<spdlog::sink_ptr>& sinks();
    static spdlog::sink_ptr& consoleSink();
    static QString& fileSinkKey();
    static spdlog::level::level_enum& configuredLevel();
    static void configureLogger(const std::shared_ptr<spdlog::logger>& logger);
    static void applySinksToLoggers();
};

class LogEntry : Noncopyable {
public:
    LogEntry(Logger::ptr logger, spdlog::level::level_enum level, spdlog::source_loc loc = {});
    LogEntry(LogEntry&& other) noexcept;
    ~LogEntry();

    template <typename T>
    LogEntry& operator<<(const T& value) {
        stream_ << value;
        return *this;
    }

    LogEntry& operator<<(const QString& value);
    LogEntry& operator<<(const QByteArray& value);

private:
    Logger::ptr logger_;
    spdlog::level::level_enum level_;
    spdlog::source_loc loc_;
    std::ostringstream stream_;
    bool active_ = true;
};

} // namespace rui
