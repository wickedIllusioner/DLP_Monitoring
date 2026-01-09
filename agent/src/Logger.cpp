#include "../include/Logger.h"
#include <QDir>
#include <QCoreApplication>
#include <QDebug>
#include <iostream>


// Инициализация статического экземпляра
Logger& Logger::instance()
{
    static Logger instance;
    return instance;
}

Logger::Logger(QObject* parent)
    : QObject(parent)
    , m_consoleOutput(true)
    , m_minLogLevel(LogLevel::INFO)
    , m_maxFileSize(10 * 1024 * 1024) // 10 MB
    , m_maxBackupFiles(5)
{}

Logger::~Logger()
{
    if (m_logFile.isOpen()) {
        info("Логгер завершает работу", "Logger");
        m_logFile.close();
    }
}

void Logger::initialize(const QString& logFilePath, bool consoleOutput)
{
    QMutexLocker locker(&m_mutex);
    m_consoleOutput = consoleOutput;

    if (!logFilePath.isEmpty()) {
        m_logFilePath = logFilePath;

        QFileInfo fileInfo(logFilePath);
        QDir dir = fileInfo.dir();
        if (!dir.exists()) {
            dir.mkpath(".");
        }

        m_logFile.setFileName(logFilePath);
        if (m_logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
            m_fileStream.setDevice(&m_logFile);
            std::cout << "Логирование в файл: " << logFilePath.toStdString() << std::endl;
        } else {
            std::cerr << "Не удалось открыть файл логов: " << logFilePath.toStdString()
                      << " - " << m_logFile.errorString().toStdString() << std::endl;
        }
    }

    std::cout << "Логгер инициализирован" << std::endl;
}

void Logger::setLogLevel(LogLevel level)
{
    {
        QMutexLocker locker(&m_mutex);
        m_minLogLevel = level;
    }
    info(QString("Установлен уровень логирования: %1").arg(levelToString(level)), "Logger");
}

void Logger::log(LogLevel level, const QString& message, const QString& source)
{
    if (!isLevelEnabled(level)) {
        return;
    }

    QString formattedMessage = formatMessage(level, message, source);
    writeToDestinations(level, formattedMessage);

    emit logMessage(level, QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz"),
                   source, message);
}

bool Logger::isLevelEnabled(LogLevel level) const
{
    return static_cast<int>(level) >= static_cast<int>(m_minLogLevel);
}

void Logger::debug(const QString& message, const QString& source)
{
    log(LogLevel::DEBUG, message, source);
}

void Logger::info(const QString& message, const QString& source)
{
    log(LogLevel::INFO, message, source);
}

void Logger::warning(const QString& message, const QString& source)
{
    log(LogLevel::WARNING, message, source);
}

void Logger::error(const QString& message, const QString& source)
{
    log(LogLevel::ERROR, message, source);
}

void Logger::fatal(const QString& message, const QString& source)
{
    log(LogLevel::FATAL, message, source);
    QCoreApplication::exit(1);
}

QString Logger::levelToString(LogLevel level)
{
    switch (level) {
    case LogLevel::DEBUG:   return "DEBUG";
    case LogLevel::INFO:    return "INFO";
    case LogLevel::WARNING: return "WARNING";
    case LogLevel::ERROR:   return "ERROR";
    case LogLevel::FATAL:    return "FATAL";
    default:               return "UNKNOWN";
    }
}

QString Logger::levelToColor(LogLevel level)
{
    switch (level) {
    case LogLevel::DEBUG:   return "\033[36m";
    case LogLevel::INFO:    return "\033[32m";
    case LogLevel::WARNING: return "\033[33m";
    case LogLevel::ERROR:   return "\033[31m";
    case LogLevel::FATAL:    return "\033[35m";
    default:               return "\033[0m";
    }
}

QString Logger::formatMessage(LogLevel level, const QString& message,
                             const QString& source) const
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz");
    QString levelStr = levelToString(level);

    QString formatted;
    if (!source.isEmpty()) {
        formatted = QString("[%1] [%2] [%3] %4")
                    .arg(timestamp, levelStr, source, message);
    } else {
        formatted = QString("[%1] [%2] %3")
                    .arg(timestamp, levelStr, message);
    }

    return formatted;
}

void Logger::writeToDestinations(LogLevel level, const QString& formattedMessage)
{
    QMutexLocker locker(&m_mutex);

    // Запись в консоль
    if (m_consoleOutput) {
        QString color = levelToColor(level);
        QString reset = "\033[0m";
        QString coloredMessage = color + formattedMessage + reset;

        std::cout << coloredMessage.toStdString() << std::endl;
        std::cout.flush();
    }

    if (m_logFile.isOpen()) {
        m_fileStream << formattedMessage << "\n";
        m_fileStream.flush();
        checkAndRotateLogFile();
    }
}

void Logger::checkAndRotateLogFile()
{
    if (m_logFile.size() > m_maxFileSize) {
        info("Размер файла логов превышен, выполняю ротацию", "Logger");

        m_logFile.close();

        for (int i = m_maxBackupFiles - 1; i > 0; --i) {
            QString oldName = QString("%1.%2").arg(m_logFilePath).arg(i);
            QString newName = QString("%1.%2").arg(m_logFilePath).arg(i + 1);

            if (QFile::exists(oldName)) {
                QFile::remove(newName);
                QFile::rename(oldName, newName);
            }
        }

        QString firstBackup = QString("%1.1").arg(m_logFilePath);
        QFile::remove(firstBackup);
        QFile::rename(m_logFilePath, firstBackup);

        if (m_logFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            info("Ротация логов завершена", "Logger");
        } else {
            error("Не удалось открыть файл логов после ротации", "Logger");
        }
    }
}