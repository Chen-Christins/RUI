#pragma once

#include <memory>
#include <sstream>

#include <QByteArray>
#include <QString>

#include <spdlog/logger.h>
#include <spdlog/spdlog.h>

#include "noncopyable.hpp"

namespace rui {

class Logger {
public:
    using ptr = std::shared_ptr<spdlog::logger>;

    static void initConsole();
    static void initFileAppender(const QString& filePath);
    static std::shared_ptr<spdlog::logger> getLogger(const QString& name);

private:
    static const char* pattern();
    static std::mutex& mutex();
    static std::vector<spdlog::sink_ptr>& sinks();
    static spdlog::sink_ptr& consoleSink();
    static QString& fileSinkPath();
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

#define TRACE(l) ::rui::LogEntry((l), spdlog::level::trace, spdlog::source_loc{__FILE__, __LINE__, nullptr})
#define DEBUG(l) ::rui::LogEntry((l), spdlog::level::debug, spdlog::source_loc{__FILE__, __LINE__, nullptr})
#define INFO(l)  ::rui::LogEntry((l), spdlog::level::info, spdlog::source_loc{__FILE__, __LINE__, nullptr})
#define WARN(l)  ::rui::LogEntry((l), spdlog::level::warn, spdlog::source_loc{__FILE__, __LINE__, nullptr})
#define ERROR(l) ::rui::LogEntry((l), spdlog::level::err, spdlog::source_loc{__FILE__, __LINE__, nullptr})
#define FATAL(l) ::rui::LogEntry((l), spdlog::level::critical, spdlog::source_loc{__FILE__, __LINE__, nullptr})
