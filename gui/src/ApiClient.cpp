#include "../include/ApiClient.h"

#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QDebug>


ApiClient::ApiClient(QObject *parent)
    : QObject(parent)
    , m_manager(new QNetworkAccessManager(this))
    , m_refreshTimer(new QTimer(this))
    , m_baseUrl("http://127.0.0.1:8080") {

    // Настройка таймера для автообновления
    m_refreshTimer->setSingleShot(false);
    connect(m_refreshTimer, &QTimer::timeout, this, &ApiClient::onAutoRefresh);

    // Обработка ответов от сервера
    connect(m_manager, &QNetworkAccessManager::finished,
            this, &ApiClient::onReplyFinished);
}

void ApiClient::setBaseUrl(const QString &url) {
    m_baseUrl = url;
}

void ApiClient::setApiKey(const QString &apiKey) {
    m_apiKey = apiKey;
}

void ApiClient::login(const QString &username, const QString &password) {
    QNetworkRequest request = createRequest("/api/v1/auth/login");

    QJsonObject json;
    json["username"] = username;
    json["password"] = password;

    QJsonDocument doc(json);
    QNetworkReply *reply = m_manager->post(request, doc.toJson());
}

void ApiClient::logout() {
    stopAutoRefresh();
    m_apiKey.clear();
}


void ApiClient::fetchPolicies() {
    QNetworkRequest request = createRequest("/api/v1/policies");
    m_manager->get(request);
}

void ApiClient::createPolicy(const QJsonObject &policy) {
    QNetworkRequest request = createRequest("/api/v1/policies");

    QJsonDocument doc(policy);
    m_manager->post(request, doc.toJson());
}

void ApiClient::updatePolicy(int id, const QJsonObject &policy) {
    QNetworkRequest request = createRequest(QString("/api/v1/policies/%1").arg(id));

    QJsonDocument doc(policy);
    m_manager->put(request, doc.toJson());
}

void ApiClient::deletePolicy(int id) {
    QNetworkRequest request = createRequest(QString("/api/v1/policies/%1").arg(id));
    m_manager->deleteResource(request);
}


void ApiClient::fetchIncidents(const QDateTime &from, const QDateTime &to) {
    QUrl url = QUrl(m_baseUrl + "/api/v1/incidents");
    QUrlQuery query;

    if (from.isValid()) {
        query.addQueryItem("from", from.toString(Qt::ISODate));
    }
    if (to.isValid()) {
        query.addQueryItem("to", to.toString(Qt::ISODate));
    }

    if (!query.isEmpty()) {
        url.setQuery(query);
    }

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    m_manager->get(request);
}

void ApiClient::updateIncidentStatus(int id, const QString &status, const QString &resolvedBy) {
    QNetworkRequest request = createRequest(QString("/api/v1/incidents/%1/status").arg(id));

    QJsonObject json;
    json["status"] = status;

    if (!resolvedBy.isEmpty()) {
        json["resolved_by"] = resolvedBy;
    }

    QJsonDocument doc(json);
    m_manager->put(request, doc.toJson());
}


void ApiClient::fetchEvents(int limit) {
    QUrl url = QUrl(m_baseUrl + "/api/v1/events");
    QUrlQuery query;

    if (limit > 0) {
        query.addQueryItem("limit", QString::number(limit));
    }

    if (!query.isEmpty()) {
        url.setQuery(query);
    }

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    m_manager->get(request);
}

void ApiClient::fetchAgents() {
    QNetworkRequest request = createRequest("/api/v1/agents");
    m_manager->get(request);
}

void ApiClient::fetchStatistics() {
    QNetworkRequest request = createRequest("/api/v1/incidents/stats");
    m_manager->get(request);
}


void ApiClient::startAutoRefresh(int intervalMs) {
    m_refreshTimer->setInterval(intervalMs);
    m_refreshTimer->start();
    qDebug() << "Auto-refresh started with interval:" << intervalMs << "ms";
}

void ApiClient::stopAutoRefresh() {
    m_refreshTimer->stop();
    qDebug() << "Auto-refresh stopped";
}

void ApiClient::onAutoRefresh() {
    fetchIncidents();
    fetchStatistics();
    fetchAgents();
}


QNetworkRequest ApiClient::createRequest(const QString &endpoint) const {
    QUrl url(m_baseUrl + endpoint);
    QNetworkRequest request(url);

    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setHeader(QNetworkRequest::UserAgentHeader, "DLP-GUI/1.0");

    if (!m_apiKey.isEmpty()) {
        request.setRawHeader("Authorization", QString("Bearer %1").arg(m_apiKey).toUtf8());
    }

    return request;
}

void ApiClient::handleError(QNetworkReply *reply) {
    QString errorMsg;

    if (reply->error() != QNetworkReply::NoError) {
        errorMsg = QString("Network error [%1]: %2")
                      .arg(reply->error())
                      .arg(reply->errorString());
    } else {
        errorMsg = "Unknown error";
    }

    qWarning() << "API Error:" << errorMsg;
    emit errorOccurred(errorMsg);
    emit networkError(reply->error(), reply->errorString());
}

void ApiClient::onReplyFinished(QNetworkReply *reply) {
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        handleError(reply);
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);

    if (doc.isNull()) {
        emit errorOccurred("Invalid JSON response from server");
        return;
    }

    QString urlPath = reply->url().path();

    if (urlPath.startsWith("/api/v1/policies")) {
        if (reply->operation() == QNetworkAccessManager::GetOperation) {
            if (doc.isArray()) {
                emit policiesFetched(doc.array());
            }
        } else if (reply->operation() == QNetworkAccessManager::PostOperation) {
            if (doc.isObject()) {
                QJsonObject response = doc.object();
                if (response.contains("id") && response.contains("message")) {
                    emit policyCreated(response);
                } else {
                    emit errorOccurred("Неверный ответ при создании политики");
                }
            }
        } else if (reply->operation() == QNetworkAccessManager::PutOperation) {
            QStringList parts = urlPath.split('/');
            if (parts.size() >= 4) {  // /api/v1/policies/{id}
                bool ok;
                int id = parts.at(3).toInt(&ok);
                if (ok) {
                    emit policyUpdated(id);
                }
            }
        } else if (reply->operation() == QNetworkAccessManager::DeleteOperation) {
            QStringList parts = urlPath.split('/');
            if (parts.size() >= 4) {  // /api/v1/policies/{id}
                bool ok;
                int id = parts.at(3).toInt(&ok);
                if (ok) {
                    emit policyDeleted(id);
                }
            }
        }
    }
    else if (urlPath.startsWith("/api/v1/incidents")) {
        if (urlPath.contains("/status") && reply->operation() == QNetworkAccessManager::PutOperation) {
            // Обновление статуса инцидента
            QStringList parts = urlPath.split('/');
            if (parts.size() > 0) {
                bool ok;
                int id = parts.at(parts.size() - 2).toInt(&ok);
                if (ok) {
                    emit incidentStatusUpdated(id);
                }
            }
        } else if (reply->operation() == QNetworkAccessManager::GetOperation) {
            if (doc.isArray()) {
                emit incidentsFetched(doc.array());
            } else if (doc.isObject() && urlPath.contains("/stats")) {
                emit statisticsFetched(doc.object());
            }
        }
    }
    else if (urlPath.startsWith("/api/v1/events")) {
        if (reply->operation() == QNetworkAccessManager::GetOperation) {
            if (doc.isArray()) {
                emit eventsFetched(doc.array());
            }
        }
    }
    else if (urlPath.startsWith("/api/v1/agents")) {
        if (reply->operation() == QNetworkAccessManager::GetOperation) {
            if (doc.isArray()) {
                emit agentsFetched(doc.array());
            }
        }
    }
    else if (urlPath == "/health") {
        qDebug() << "Health check OK";
    }
    else {
        qWarning() << "Unhandled API response from:" << urlPath;
    }
}