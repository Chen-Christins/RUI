#pragma once

#include <QDir>
#include <QString>

#include <log4qt/logger.h>
#include <log4qt/consoleappender.h>
#include <log4qt/fileappender.h>
#include <log4qt/patternlayout.h>

namespace rui {

class Logger {
public:
    static void initConsole() {
        auto* layout = new Log4Qt::PatternLayout("%d{ISO8601} [%t] %-5p %c - %m%n");
        layout->activateOptions();

        auto* consoleAppender = new Log4Qt::ConsoleAppender(
            layout, Log4Qt::ConsoleAppender::StdOut);
        consoleAppender->activateOptions();

        auto* root = Log4Qt::Logger::rootLogger();
        root->addAppender(consoleAppender);
        root->setLevel(Log4Qt::Level::DEBUG_INT);
    }

    static void initFileAppender(const QString& logDir) {
        if (logDir.isEmpty()) {
            return;
        }

        QDir dir(logDir);
        if (!dir.exists()) {
            dir.mkpath(".");
        }

        auto* layout = new Log4Qt::PatternLayout("%d{ISO8601} [%t] %-5p %c - %m%n");
        layout->activateOptions();

        auto* fileAppender = new Log4Qt::FileAppender(layout,
            QDir::cleanPath(logDir + "/sgui.log"), true);
        fileAppender->activateOptions();

        Log4Qt::Logger::rootLogger()->addAppender(fileAppender);
    }

    static Log4Qt::Logger* getLogger(const QString& name) {
        return Log4Qt::Logger::logger(name);
    }
};

} // namespace rui

#define TRACE(l) l->trace()
#define DEBUG(l) l->debug()
#define INFO(l)  l->info()
#define WARN(l)  l->warn()
#define ERROR(l) l->error()
#define FATAL(l) l->fatal()
