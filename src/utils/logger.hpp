#pragma once

#include <QDir>
#include <QFileInfo>
#include <QString>
#include <QByteArray>

#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <spdlog/logger.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

namespace rui {

class Logger {
public:
    using ptr = std::shared_ptr<spdlog::logger>;

    static void initConsole() {
        std::lock_guard<std::mutex> lock(mutex());
        if (!consoleSink()) {
            consoleSink() = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            consoleSink()->set_pattern(pattern());
            sinks().push_back(consoleSink());
        }
        applySinksToLoggers();
    }

    static void initFileAppender(const QString& filePath) {
        if (filePath.isEmpty()) {
            return;
        }

        const auto path = QDir::cleanPath(filePath);
        {
            QFileInfo fi(path);
            if (fi.exists() && fi.isDir()) {
                QDir d(path);
                d.removeRecursively();
            }
            QDir().mkpath(fi.absolutePath());
        }

        std::lock_guard<std::mutex> lock(mutex());
        if (fileSinkPath() == path) {
            return;
        }

        fileSinkPath() = path;
        auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
            path.toStdString(), true);
        sink->set_pattern(pattern());
        sinks().push_back(std::move(sink));
        applySinksToLoggers();
    }

    static std::shared_ptr<spdlog::logger> getLogger(const QString& name) {
        const auto loggerName = name.toStdString();

        std::lock_guard<std::mutex> lock(mutex());
        if (auto logger = spdlog::get(loggerName)) {
            return logger;
        }

        auto logger = std::make_shared<spdlog::logger>(
            loggerName, sinks().begin(), sinks().end());
        configureLogger(logger);
        spdlog::register_logger(logger);
        return logger;
    }

private:
    static const char* pattern() {
        return "%Y-%m-%d %H:%M:%S.%f\t%t\t[%L]\t[%n]\t%@ %v";
    }

    static std::mutex& mutex() {
        static std::mutex mutex;
        return mutex;
    }

    static std::vector<spdlog::sink_ptr>& sinks() {
        static std::vector<spdlog::sink_ptr> sinks;
        return sinks;
    }

    static spdlog::sink_ptr& consoleSink() {
        static spdlog::sink_ptr sink;
        return sink;
    }

    static QString& fileSinkPath() {
        static QString path;
        return path;
    }

    static void configureLogger(const std::shared_ptr<spdlog::logger>& logger) {
        logger->set_level(spdlog::level::trace);
        logger->flush_on(spdlog::level::err);
    }

    static void applySinksToLoggers() {
        spdlog::apply_all([](const std::shared_ptr<spdlog::logger>& logger) {
            logger->sinks() = sinks();
            configureLogger(logger);
        });
    }
};

class LogEntry {
public:
    LogEntry(Logger::ptr logger, spdlog::level::level_enum level, spdlog::source_loc loc = {})
        : logger_(std::move(logger)), level_(level), loc_(loc) {}

    LogEntry(LogEntry&& other) noexcept
        : logger_(std::move(other.logger_)),
          level_(other.level_),
          loc_(other.loc_),
          stream_(std::move(other.stream_)),
          active_(std::exchange(other.active_, false)) {}

    LogEntry(const LogEntry&) = delete;
    LogEntry& operator=(const LogEntry&) = delete;
    LogEntry& operator=(LogEntry&&) = delete;

    ~LogEntry() {
        if (active_ && logger_) {
            logger_->log(loc_, level_, "{}", stream_.str());
        }
    }

    template <typename T>
    LogEntry& operator<<(const T& value) {
        stream_ << value;
        return *this;
    }

    LogEntry& operator<<(const QString& value) {
        stream_ << value.toStdString();
        return *this;
    }

    LogEntry& operator<<(const QByteArray& value) {
        stream_ << std::string(value.constData(), static_cast<size_t>(value.size()));
        return *this;
    }

private:
    Logger::ptr logger_;
    spdlog::level::level_enum level_;
    spdlog::source_loc loc_;
    std::ostringstream stream_;
    bool active_ = true;
};

} // namespace rui

#ifdef ERROR
#undef ERROR
#endif

#define TRACE(l) ::rui::LogEntry((l), spdlog::level::trace, spdlog::source_loc{__FILE__, __LINE__, nullptr})
#define DEBUG(l) ::rui::LogEntry((l), spdlog::level::debug, spdlog::source_loc{__FILE__, __LINE__, nullptr})
#define INFO(l)  ::rui::LogEntry((l), spdlog::level::info, spdlog::source_loc{__FILE__, __LINE__, nullptr})
#define WARN(l)  ::rui::LogEntry((l), spdlog::level::warn, spdlog::source_loc{__FILE__, __LINE__, nullptr})
#define ERROR(l) ::rui::LogEntry((l), spdlog::level::err, spdlog::source_loc{__FILE__, __LINE__, nullptr})
#define FATAL(l) ::rui::LogEntry((l), spdlog::level::critical, spdlog::source_loc{__FILE__, __LINE__, nullptr})
