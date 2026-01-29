#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonArray>

class NetworkManager : public QObject {
    Q_OBJECT

public:
    explicit NetworkManager(QObject* parent = nullptr);
    ~NetworkManager();

    void registerAgent(const QString& agentId, const QString& hostname,
                       const QString& ipAddr = "", const QString& osInfo = "");
    void sendHeartbeat(const QString& agentId);
    void sendEvent(const QJsonObject& event);
    void getPoliciesForAgent();
    QNetworkAccessManager* getManager() const { return m_manager; }

    QString serverUrl() const { return m_serverUrl; }
    void setServerUrl(const QString& url);
    void setTimeout(int msec);

signals:
    void agentRegistered(const QJsonObject& resp);
    void heartbeatSent(bool success);
    void eventSent(const QJsonObject& resp);
    void policiesReceived(const QJsonArray& policies);
    void errorOccurred(const QString& error);

private slots:
    void onReplyFinished(QNetworkReply* reply);

private:
    QNetworkRequest createRequest(const QString& endpoint) const;
    void handleError(const QString& context, const QString& error);
    QByteArray prepareJson(const QJsonObject& data) const;

    QNetworkAccessManager* m_manager;
    QString m_serverUrl;
    int m_timeout;
    QList<QNetworkReply*> m_activeReplies;
};

#endif // NETWORKMANAGER_H