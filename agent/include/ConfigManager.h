#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariant>


class ConfigManager : public QObject {
    Q_OBJECT

public:
    static ConfigManager& instance();

    bool loadConfig(const QString& configPath = "");
    QVariant get(const QString& key, const QVariant& defVal = QVariant()) const;
    void set(const QString& key, const QVariant& value);
    bool saveConfig();

    QString agentId() const;
    QString serverUrl() const;
    QStringList monitorDirs() const;
    QString logLevel() const;
    QString logFile() const;

    bool isLoaded() const { return m_loaded; }
    QString configPath() const { return m_configPath; }

signals:
    void configChanged(const QString& key, const QVariant& value);

private:
    explicit ConfigManager(QObject* parent = nullptr);
    ~ConfigManager() = default;

    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    void initDefaults();
    QString normalizePath(const QString& path) const;
    QString generateAgentId() const;
    QString findConfigFile() const;

    QString m_configPath;
    bool m_loaded;
    QVariantMap m_settings;

};

#endif
