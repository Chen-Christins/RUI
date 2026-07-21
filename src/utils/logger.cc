#include "logger.hpp"

#include <QDir>
#include <QFileInfo>

#include <mutex>
#include <vector>

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace rui {

static auto logger = Logger::getLogger("App");

void Logger::initConsole() {
#ifndef NDEBUG
    std::lock_guard<std::mutex> lock(mutex());
    if (!consoleSink()) {
        consoleSink() = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        consoleSink()->set_pattern(pattern());
        sinks().push_back(consoleSink());
    }
    applySinksToLoggers();
#endif
}

void Logger::initFileAppender(const QString& filePath) {
    DEBUG(logger) << "Initializing file appender with path: " << filePath;
    
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
    auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path.toStdString(), true);
    sink->set_pattern(pattern());
    sinks().push_back(std::move(sink));
    applySinksToLoggers();
}

std::shared_ptr<spdlog::logger> Logger::getLogger(const QString& name) {
    const auto loggerName = name.toStdString();

    std::lock_guard<std::mutex> lock(mutex());
    if (auto logger = spdlog::get(loggerName)) {
        return logger;
    }

    auto logger = std::make_shared<spdlog::logger>(loggerName, sinks().begin(), sinks().end());
    configureLogger(logger);
    spdlog::register_logger(logger);
    return logger;
}

const char* Logger::pattern() {
    return "%Y-%m-%d %H:%M:%S.%f\t%t\t[%L]\t[%n]\t%@ %v";
}

std::mutex& Logger::mutex() {
    static std::mutex mutex;
    return mutex;
}

std::vector<spdlog::sink_ptr>& Logger::sinks() {
    static std::vector<spdlog::sink_ptr> sinks;
    return sinks;
}

spdlog::sink_ptr& Logger::consoleSink() {
    static spdlog::sink_ptr sink;
    return sink;
}

QString& Logger::fileSinkPath() {
    static QString path;
    return path;
}

void Logger::configureLogger(const std::shared_ptr<spdlog::logger>& logger) {
#ifdef NDEBUG
    logger->set_level(spdlog::level::info);
    logger->flush_on(spdlog::level::err);
#else
    logger->set_level(spdlog::level::trace);
    logger->flush_on(spdlog::level::trace);
#endif
}

void Logger::applySinksToLoggers() {
    spdlog::apply_all([](const std::shared_ptr<spdlog::logger>& logger) {
        logger->sinks() = sinks();
        configureLogger(logger);
    });
}

LogEntry::LogEntry(Logger::ptr logger, spdlog::level::level_enum level, spdlog::source_loc loc)
    : logger_(std::move(logger))
    , level_(level)
    , loc_(loc) {
}

LogEntry::LogEntry(LogEntry&& other) noexcept
    : logger_(std::move(other.logger_))
    , level_(other.level_)
    , loc_(other.loc_)
    , stream_(std::move(other.stream_))
    , active_(std::exchange(other.active_, false)) {
}

LogEntry::~LogEntry() {
    if (active_ && logger_) {
        logger_->log(loc_, level_, "{}", stream_.str());
    }
}

LogEntry& LogEntry::operator<<(const QString& value) {
    stream_ << value.toStdString();
    return *this;
}

LogEntry& LogEntry::operator<<(const QByteArray& value) {
    stream_ << std::string(value.constData(), static_cast<size_t>(value.size()));
    return *this;
}

} // namespace rui
