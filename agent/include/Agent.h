#ifndef AGENT_H
#define AGENT_H

#include <QObject>
#include "ConfigManager.h"
#include "NetworkManager.h"
#include "FileMonitor.h"
#include "PolicyChecker.h"
#include "ContentAnalyzer.h"
#include "EventQueue.h"

class Agent : public QObject
{
    Q_OBJECT

public:
    explicit Agent(QObject* parent = nullptr);
    ~Agent();

    bool start();
    void stop();

signals:
    void started();
    void stopped();
    void errorOccurred(const QString& error);

private slots:
    void onFileCreated(const QString& filePath, qint64 size);
    void onFileModified(const QString& filePath, qint64 size);
    void onFileDeleted(const QString& filePath);
    void onFileAnalyzed(const QString& filePath, bool hasViolations,
                       const QList<PolicyMatch>& matches, qint64 size);
    void onPoliciesReceived(const QJsonArray& policies);
    void onHeartbeatSent(bool success);
    void onEventSent(const QJsonObject& resp);
    void onNetworkError(const QString& error);

private:
    // Вспомогательные методы
    void registerAgent();
    void loadPolicies();
    void sendHeartbeat();
    void sendEvent(const QString& filePath, const QString& content, const QString& eventType,
                   bool isViolation, const QList<PolicyMatch>& matches);
    void analyzeAndSendEvent(const QString& filePath, qint64 size, const QString& eventType);

    QTimer* m_heartbeatTimer;
    QHash<QString,QString> m_fileEventTypes;
    QSet<QString> m_violationFiles;

    ConfigManager& m_config;
    NetworkManager m_network;
    FileMonitor m_monitor;
    PolicyChecker m_checker;
    ContentAnalyzer m_analyzer;
    EventQueue m_eventQueue;

    bool m_running;
};

#endif //AGENT_H
