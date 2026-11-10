#include "logger.hpp"

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <QDir>
#include <QFileInfo>
#include <mutex>
#include <string>
#include <vector>

namespace rui {

static auto logger = Logger::getLogger("App");

namespace {

spdlog::level::level_enum defaultLevel()
{
#ifdef NDEBUG
    return spdlog::level::info;
#else
    return spdlog::level::trace;
#endif
}

spdlog::level::level_enum parseLevel(const QString& level)
{
    const QString text = level.trimmed().toLower();
    if (text == "trace") {
        return spdlog::level::trace;
    }
    if (text == "debug") {
        return spdlog::level::debug;
    }
    if (text == "info") {
        return spdlog::level::info;
    }
    if (text == "warn" || text == "warning") {
        return spdlog::level::warn;
    }
    if (text == "error" || text == "err") {
        return spdlog::level::err;
    }
    if (text == "fatal" || text == "critical") {
        return spdlog::level::critical;
    }
    if (text == "off") {
        return spdlog::level::off;
    }
    return defaultLevel();
}

QString levelName(spdlog::level::level_enum level)
{
    const auto name = spdlog::level::to_string_view(level);
    return QString::fromUtf8(name.data(), static_cast<qsizetype>(name.size()));
}

} // namespace

void Logger::initConsole()
{
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

void Logger::initFileAppender(const QString& filePath, std::size_t maxSizeBytes, std::size_t maxFiles)
{
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
    if (maxFiles == 0) {
        maxFiles = 1;
    }

    const QString key = QString("%1|%2|%3").arg(path).arg(static_cast<qulonglong>(maxSizeBytes)).arg(static_cast<qulonglong>(maxFiles));
    if (fileSinkKey() == key) {
        return;
    }

    fileSinkKey() = key;
    spdlog::sink_ptr sink;
    if (maxSizeBytes > 0) {
        sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(path.toStdString(), maxSizeBytes, maxFiles);
    } else {
        sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path.toStdString(), true);
    }
    sink->set_pattern(pattern());
    sinks().push_back(std::move(sink));
    applySinksToLoggers();
}

void Logger::setLevel(const QString& level)
{
    std::lock_guard<std::mutex> lock(mutex());
    configuredLevel() = parseLevel(level);
    applySinksToLoggers();
}

QString Logger::defaultLevelName()
{
    return levelName(defaultLevel());
}

std::shared_ptr<spdlog::logger> Logger::getLogger(const QString& name)
{
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

const char* Logger::pattern()
{
    return "%Y-%m-%d %H:%M:%S.%f\t%t\t[%L]\t[%n]\t%@ %v";
}

std::mutex& Logger::mutex()
{
    static std::mutex mutex;
    return mutex;
}

std::vector<spdlog::sink_ptr>& Logger::sinks()
{
    static std::vector<spdlog::sink_ptr> sinks;
    return sinks;
}

spdlog::sink_ptr& Logger::consoleSink()
{
    static spdlog::sink_ptr sink;
    return sink;
}

QString& Logger::fileSinkKey()
{
    static QString key;
    return key;
}

spdlog::level::level_enum& Logger::configuredLevel()
{
    static spdlog::level::level_enum level = defaultLevel();
    return level;
}

void Logger::configureLogger(const std::shared_ptr<spdlog::logger>& logger)
{
    logger->set_level(configuredLevel());
#ifdef NDEBUG
    logger->flush_on(spdlog::level::err);
#else
    logger->flush_on(spdlog::level::trace);
#endif
}

void Logger::applySinksToLoggers()
{
    spdlog::apply_all([](const std::shared_ptr<spdlog::logger>& logger) {
        logger->sinks() = sinks();
        configureLogger(logger);
    });
}

LogEntry::LogEntry(Logger::ptr logger, spdlog::level::level_enum level, spdlog::source_loc loc)
    : logger_(std::move(logger))
    , level_(level)
    , loc_(loc)
{
}

LogEntry::LogEntry(LogEntry&& other) noexcept
    : logger_(std::move(other.logger_))
    , level_(other.level_)
    , loc_(other.loc_)
    , stream_(std::move(other.stream_))
    , active_(std::exchange(other.active_, false))
{
}

LogEntry::~LogEntry()
{
    if (active_ && logger_) {
        logger_->log(loc_, level_, "{}", stream_.str());
    }
}

LogEntry& LogEntry::operator<<(const QString& value)
{
    stream_ << value.toStdString();
    return *this;
}

LogEntry& LogEntry::operator<<(const QByteArray& value)
{
    stream_ << std::string(value.constData(), static_cast<size_t>(value.size()));
    return *this;
}

} // namespace rui
