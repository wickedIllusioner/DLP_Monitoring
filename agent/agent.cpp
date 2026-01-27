// main.cpp
#include "include/Agent.h"
#include "include/Logger.h"
#include "include/ConfigManager.h"
#include <QCoreApplication>
#include <QCommandLineParser>
#include <iostream>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("DLP Agent");
    QCoreApplication::setApplicationVersion("1.0");

    // Парсинг аргументов командной строки
    QCommandLineParser parser;
    parser.setApplicationDescription("DLP Agent - мониторинг файлов на наличие конфиденциальной информации");
    parser.addHelpOption();
    parser.addVersionOption();

    // Опция для указания конфиг-файла
    QCommandLineOption configOption(
        {"c", "config"},
        "Путь к конфигурационному файлу",
        "config-path"
    );
    parser.addOption(configOption);

    // Опция для указания имени агента
    QCommandLineOption nameOption(
        {"n", "name"},
        "Имя агента (по умолчанию: имя пользователя системы)",
        "agent-name"
    );
    parser.addOption(nameOption);

    // Аргументы: директории для мониторинга
    parser.addPositionalArgument(
        "directories",
        "Директории для мониторинга (можно указать несколько)",
        "[directories...]"
    );

    parser.process(app);

    const QStringList directories = parser.positionalArguments();
    if (directories.isEmpty()) {
        std::cerr << "Ошибка: необходимо указать хотя бы одну директорию для мониторинга" << std::endl;
        std::cerr << "Использование: " << argv[0] << " [опции] <директория1> [директория2 ...]" << std::endl;
        return 1;
    }

    // Инициализация логгера
    Logger::instance().initialize("", true);
    Logger::instance().setLogLevel(LogLevel::INFO);

    LOG_INFO("Запуск DLP агента...");

    // Загрузка конфигурации
    ConfigManager& config = ConfigManager::instance();
    if (parser.isSet(configOption)) {
        if (!config.loadConfig(parser.value(configOption))) {
            LOG_ERROR("Не удалось загрузить конфигурацию");
            return 1;
        }
    } else {
        if (!config.loadConfig()) {
            LOG_WARNING("Использую конфигурацию по умолчанию");
        }
    }

    // Установка имени агента
    QString agentName;
    if (parser.isSet(nameOption)) {
        agentName = parser.value(nameOption);
    } else {
        agentName = qEnvironmentVariable("USER", qEnvironmentVariable("USERNAME", "unknown"));
    }
    config.set("agent/hostname", agentName);
    LOG_INFO(QString("Имя агента: %1").arg(agentName));

    // Установка директорий мониторинга
    config.set("monitoring/directories", directories);
    LOG_INFO(QString("Мониторинг директорий: %1").arg(directories.join(", ")));

    // Создание и запуск агента
    Agent agent;

    QObject::connect(&app, &QCoreApplication::aboutToQuit, [&agent]() {
        LOG_INFO("Завершение работы агента...");
        agent.stop();
    });

    QObject::connect(&agent, &Agent::errorOccurred, [](const QString& error) {
        LOG_ERROR(QString("Ошибка агента: %1").arg(error));
    });

    if (!agent.start()) {
        LOG_ERROR("Не удалось запустить агент");
        return 1;
    }

    LOG_INFO("DLP агент успешно запущен. Для остановки нажмите Ctrl+C");

    return app.exec();
}