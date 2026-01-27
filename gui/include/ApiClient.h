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

    // Базовые методы
    void setBaseUrl(const QString &url);
    void setApiKey(const QString &apiKey);

    // Методы для политик
    void fetchPolicies();
    void createPolicy(const QJsonObject &policy);
    void updatePolicy(int id, const QJsonObject &policy);
    void deletePolicy(int id);

    // Методы для инцидентов
    void fetchIncidents(const QDateTime &from = QDateTime(),
                        const QDateTime &to = QDateTime());
    void updateIncidentStatus(int id, const QString &status,
                              const QString &resolvedBy = "");

    void fetchEvents(int limit = 100);
    void fetchAgents();
    void fetchStatistics();

    // Периодическое обновление
    void startAutoRefresh(int intervalMs = 30000);  // 30 секунд по умолчанию
    void stopAutoRefresh();

signals:
    // Сигналы успешного выполнения
    void policiesFetched(const QJsonArray &policies);
    void policyCreated(const QJsonObject &policy);
    void policyUpdated(int id);
    void policyDeleted(int id);

    void incidentsFetched(const QJsonArray &incidents);
    void incidentStatusUpdated(int id);

    void eventsFetched(const QJsonArray &events);
    void agentsFetched(const QJsonArray &agents);
    void statisticsFetched(const QJsonObject &stats);

    // Сигналы ошибок
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