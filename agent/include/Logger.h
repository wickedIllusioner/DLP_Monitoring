// agent/include/Logger.h
#ifndef LOGGER_H
#define LOGGER_H

#include <QObject>
#include <QString>
#include <QFile>
#include <QTextStream>
#include <QMutex>
#include <QDateTime>

/**
 * @brief Уровни логирования
 */
enum class LogLevel {
    DEBUG   = 0,
    INFO    = 1,
    WARNING = 2,
    ERROR   = 3,
    FATAL   = 4
};

/**
 * @brief Класс для логирования в файл и консоль
 * 
 * Поддерживает разные уровни логирования, ротацию файлов,
 * и потокобезопасную запись.
 */
class Logger : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Получить единственный экземпляр логгера (Singleton)
     */
    static Logger& instance();

    /**
     * @brief Инициализировать логгер
     * @param logFilePath Путь к файлу логов
     * @param consoleOutput Выводить ли в консоль
     */
    void initialize(const QString& logFilePath = "", bool consoleOutput = true);

    /**
     * @brief Установить минимальный уровень логирования
     * @param level Уровень логирования
     */
    void setLogLevel(LogLevel level);

    /**
     * @brief Записать сообщение в лог
     * @param level Уровень сообщения
     * @param message Текст сообщения
     * @param source Источник сообщения (класс/функция)
     */
    void log(LogLevel level, const QString& message, const QString& source = "");

    /**
     * @brief Проверить, включен ли указанный уровень
     * @param level Уровень для проверки
     */
    bool isLevelEnabled(LogLevel level) const;

    // Вспомогательные методы для разных уровней
    void debug(const QString& message, const QString& source = "");
    void info(const QString& message, const QString& source = "");
    void warning(const QString& message, const QString& source = "");
    void error(const QString& message, const QString& source = "");
    void fatal(const QString& message, const QString& source = "");

    /**
     * @brief Получить строковое представление уровня логирования
     */
    static QString levelToString(LogLevel level);

    /**
     * @brief Получить цвет для уровня логирования (для консоли)
     */
    static QString levelToColor(LogLevel level);

signals:
    /**
     * @brief Сигнал о новом сообщении в логе (можно использовать в GUI)
     */
    void logMessage(LogLevel level, const QString& timestamp, 
                    const QString& source, const QString& message);

private:
    explicit Logger(QObject* parent = nullptr);
    ~Logger() override;

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    /**
     * @brief Записать сообщение во все назначения (файл, консоль)
     */
    void writeToDestinations(LogLevel level, const QString& formattedMessage);

    /**
     * @brief Отформатировать сообщение для записи
     */
    QString formatMessage(LogLevel level, const QString& message, 
                         const QString& source) const;

    /**
     * @brief Проверить и выполнить ротацию логов при необходимости
     */
    void checkAndRotateLogFile();

    // Члены класса
    QFile m_logFile;
    QTextStream m_fileStream;
    bool m_consoleOutput;
    LogLevel m_minLogLevel;
    QString m_logFilePath;
    qint64 m_maxFileSize;
    int m_maxBackupFiles;
    mutable QMutex m_mutex;
};

// Макросы для удобного использования
#define LOG_DEBUG(message) Logger::instance().debug(message, QString(__FUNCTION__))
#define LOG_INFO(message) Logger::instance().info(message, QString(__FUNCTION__))
#define LOG_WARNING(message) Logger::instance().warning(message, QString(__FUNCTION__))
#define LOG_ERROR(message) Logger::instance().error(message, QString(__FUNCTION__))
#define LOG_FATAL(message) Logger::instance().fatal(message, QString(__FUNCTION__))

#endif // LOGGER_H