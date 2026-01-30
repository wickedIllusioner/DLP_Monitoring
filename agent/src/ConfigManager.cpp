#include "../include/ConfigManager.h"
#include "../include/Logger.h"

#include <QSettings>
#include <QCoreApplication>
#include <QDir>
#include <QStandardPaths>
#include <QHostInfo>
#include <QRegularExpression>


ConfigManager &ConfigManager::instance() {
    static ConfigManager instance;
    return instance;
}


ConfigManager::ConfigManager(QObject *parent)
    : QObject(parent)
    , m_loaded(false) {
    initDefaults();
}


void ConfigManager::initDefaults() {
    m_settings["agent/id"] = generateAgentId();
    m_settings["agent/hostname"] = QHostInfo::localHostName();
    m_settings["agent/scan_interval"] = 60;
    m_settings["agent/max_file_size"] = 10*1024*1024;

    m_settings["monitoring/dirs"] = QStringList()
        << QDir::homePath() + "/Documents"
        << QDir::homePath() + "/Desktop";
    m_settings["monitoring/exclude_patterns"] = QStringList()
        << "*.tmp" << "*.log" << "*.cache";
    m_settings["monitoring/recursive"] = true;

    m_settings["server/url"] = "http://127.0.0.1:8080";
    m_settings["server/heartbeat_interval"] = 300;
    m_settings["server/timeout"] = 30000;

    m_settings["logs/level"] = "info";
    m_settings["logs/file"] = QDir::tempPath() + "/dlp_agent.log";
}


bool ConfigManager::loadConfig(const QString& configPath) {
    QString path = configPath.isEmpty() ? findConfigFile() : configPath;

    if (path.isEmpty() || !QFile::exists(path)) {
        LOG_WARNING("Конфигурационный файл не найден, использую настройки по умолчанию");
        m_loaded = true;
        return true;
    }

    QSettings settings(path, QSettings::IniFormat);

    if (settings.status() != QSettings::NoError) {
        LOG_ERROR(QString("Ошибка чтения конфигурации: %1").arg(path));
        return false;
    }

    m_configPath = path;

    QStringList groups = settings.childGroups();
    for (const QString& group : groups) {
        settings.beginGroup(group);
        QStringList keys = settings.childKeys();
        for (const QString& key : keys) {
            QString fullKey = group + "/" + key;
            m_settings[fullKey] = settings.value(key, m_settings.value(fullKey));
        }
        settings.endGroup();
    }

    QStringList dirs = get("monitoring/directories").toStringList();
    for (int i = 0; i < dirs.size(); ++i) {
        dirs[i] = normalizePath(dirs[i]);
    }
    set("monitoring/directories", dirs);

    m_loaded = true;
    LOG_INFO(QString("Конфигурация загружена: %1").arg(path));
    return true;
}


QVariant ConfigManager::get(const QString& key, const QVariant& defaultValue) const {
    return m_settings.value(key, defaultValue);
}


void ConfigManager::set(const QString& key, const QVariant& value) {
    if (m_settings.value(key) != value) {
        m_settings[key] = value;
        emit configChanged(key, value);
        LOG_DEBUG(QString("Настройка изменена: %1 = %2").arg(key).arg(value.toString()));
    }
}


bool ConfigManager::saveConfig() {
    if (m_configPath.isEmpty()) {
        LOG_ERROR("Не указан путь для сохранения конфигурации");
        return false;
    }

    QSettings settings(m_configPath, QSettings::IniFormat);

    // Группировка настроек по префиксам
    QMap<QString, QVariantMap> grouped;
    for (auto it = m_settings.constBegin(); it != m_settings.constEnd(); ++it) {
        QStringList parts = it.key().split('/');
        if (parts.size() >= 2) {
            QString group = parts[0];
            QString key = parts[1];
            grouped[group][key] = it.value();
        }
    }

    // Сохранение по группам
    for (auto groupIt = grouped.constBegin(); groupIt != grouped.constEnd(); ++groupIt) {
        settings.beginGroup(groupIt.key());
        for (auto keyIt = groupIt.value().constBegin(); keyIt != groupIt.value().constEnd(); ++keyIt) {
            settings.setValue(keyIt.key(), keyIt.value());
        }
        settings.endGroup();
    }
    settings.sync();

    if (settings.status() == QSettings::NoError) {
        LOG_INFO("Конфигурация сохранена");
        return true;
    }
    LOG_ERROR("Ошибка сохранения конфигурации");
    return false;
}


QString ConfigManager::agentId() const {
    return get("agent/id").toString();
}


QString ConfigManager::serverUrl() const {
    return get("server/url").toString();
}


QStringList ConfigManager::monitorDirs() const {
    return get("monitoring/directories").toStringList();
}


QString ConfigManager::logLevel() const {
    return get("logging/level").toString();
}


QString ConfigManager::logFile() const {
    return normalizePath(get("logging/file").toString());
}


QString ConfigManager::normalizePath(const QString& path) const {
    QString normalized = path.trimmed();

    if (normalized.startsWith("~/")) {
        normalized = QDir::homePath() + normalized.mid(1);
    }

    return QFileInfo(normalized).absoluteFilePath();
}


QString ConfigManager::generateAgentId() const {
    QString hostname = QHostInfo::localHostName();
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMddhhmmss");
    return QString("%1-%2").arg(hostname).arg(timestamp);
}


QString ConfigManager::findConfigFile() const {
    QStringList searchPaths;

    searchPaths << QDir::currentPath() + "/agent.conf";
    searchPaths << QCoreApplication::applicationDirPath() + "/agent.conf";
    searchPaths << QDir::homePath() + "/.dlp-agent.conf";

    for (const QString& path : searchPaths) {
        if (QFile::exists(path)) {
            return path;
        }
    }

    return QString();
}


QString ConfigManager::getOsInfo() const {
#ifdef Q_OS_WIN
    return "Windows";
#elif defined(Q_OS_MAC)
    return "macOS";
#elif defined(Q_OS_LINUX)
    QFile osRelease("/etc/os-release");
    if (osRelease.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&osRelease);
        QString content = in.readAll();

        QRegularExpression regex("PRETTY_NAME=\"([^\"]+)\"");
        QRegularExpressionMatch match = regex.match(content);
        if (match.hasMatch()) {
            return match.captured(1);
        }

        regex.setPattern("NAME=\"([^\"]+)\"");
        match = regex.match(content);
        if (match.hasMatch()) {
            return match.captured(1);
        }
    }
    return "Linux";
#elif defined(Q_OS_UNIX)
    return "Unix";
#else
    return "Unknown";
#endif
}