#ifndef APICLIENT_H
#define APICLIENT_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonArray>
#include <QJsonObject>
#include <QTimer>

class ApiClient : public QObject {
    Q_OBJECT

public:
    explicit ApiClient(QObject *parent = nullptr);

    void setBaseUrl(const QString &url);
    void setApiKey(const QString &apiKey);
    QString getBaseUrl() { return m_baseUrl; }

    void fetchPolicies();
    void createPolicy(const QJsonObject &policy);
    void updatePolicy(int id, const QJsonObject &policy);
    void deletePolicy(int id);

    void fetchIncidents(const QDateTime &from = QDateTime(),
                        const QDateTime &to = QDateTime());
    void updateIncidentStatus(int id, const QString &status,
                              const QString &resolvedBy = "");

    void fetchEvents(int limit = 100);
    void fetchAgents();
    void fetchStatistics();

    void startAutoRefresh(int intervalMs = 30000);
    void stopAutoRefresh();

    void login(const QString &username, const QString &password);
    void logout();

signals:
    void policiesFetched(const QJsonArray &policies);
    void policyCreated(const QJsonObject &policy);
    void policyUpdated(int id);
    void policyDeleted(int id);

    void incidentsFetched(const QJsonArray &incidents);
    void incidentStatusUpdated(int id);

    void eventsFetched(const QJsonArray &events);
    void agentsFetched(const QJsonArray &agents);
    void statisticsFetched(const QJsonObject &stats);

    void errorOccurred(const QString &error);
    void networkError(int code, const QString &message);

private slots:
    void onReplyFinished(QNetworkReply *reply);
    void onAutoRefresh();

private:
    QNetworkAccessManager *m_manager;
    QTimer *m_refreshTimer;
    QString m_baseUrl;
    QString m_apiKey;

    QNetworkRequest createRequest(const QString &endpoint) const;
    void handleError(QNetworkReply *reply);
};

#endif //APICLIENT_H