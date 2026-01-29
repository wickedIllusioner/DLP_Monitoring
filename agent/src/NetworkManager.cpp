#include "../include/NetworkManager.h"
#include "../include/Logger.h"
#include "../include/ConfigManager.h"
#include <QUrl>
#include <QJsonDocument>
#include <QTimer>
#include <QNetworkRequest>

NetworkManager::NetworkManager(QObject *parent)
    : QObject(parent)
    , m_manager(new QNetworkAccessManager(this))
    , m_timeout(10000) {
    // Получение настроек из конфига
    m_serverUrl = ConfigManager::instance().get("server/url", "http://127.0.0.1:8080").toString();
    m_timeout = ConfigManager::instance().get("server/timeout", 10000).toInt();

    if (m_serverUrl.endsWith('/')) {
        m_serverUrl.chop(1);
    }

    LOG_INFO(QString("Инициализация NetworkManager"));
    LOG_INFO(QString("URL сервера: %1").arg(m_serverUrl));
    LOG_INFO(QString("Таймаут: %1 мс").arg(m_timeout));

    // Проверка сетевого соединения
    QNetworkRequest request(QUrl(m_serverUrl + "/health"));
    request.setTransferTimeout(m_timeout);

    QNetworkReply *reply  = m_manager->get(request);
    connect(reply, &QNetworkReply::finished, this, [reply]() {
        if (reply->error() != QNetworkReply::NoError) {
            LOG_WARNING(QString("Сеть недоступна: %1").arg(reply->errorString()));
        } else {
            LOG_INFO("Сетевое соединение доступно");
        }
        reply->deleteLater();
    });

    connect(m_manager, &QNetworkAccessManager::finished,
            this, &NetworkManager::onReplyFinished);
}

NetworkManager::~NetworkManager() {
    LOG_DEBUG("Очистка NetworkManager");
    for (QNetworkReply* reply : m_activeReplies) {
        if (reply && reply->isRunning()) {
            reply->abort();
            reply->deleteLater();
        }
    }
    m_activeReplies.clear();
}

void NetworkManager::setServerUrl(const QString &url) {
    QString newUrl = url;
    if (newUrl.endsWith('/')) {
        newUrl.chop(1);
    }

    if (m_serverUrl != newUrl) {
        m_serverUrl = newUrl;
        LOG_INFO(QString("URL сервера изменен: %1").arg(m_serverUrl));
    }
}

void NetworkManager::setTimeout(int msec) {
    if (m_timeout != msec) {
        m_timeout = msec;
        LOG_DEBUG(QString("Таймаут изменен: %1 мс").arg(m_timeout));
    }
}

QByteArray NetworkManager::prepareJson(const QJsonObject& data) const {
    QJsonDocument doc(data);
    return doc.toJson(QJsonDocument::Compact);
}

QNetworkRequest NetworkManager::createRequest(const QString& endpoint) const {
    QString fullUrl = m_serverUrl + endpoint;
    QUrl url(fullUrl);

    if (!url.isValid()) {
        LOG_ERROR(QString("Неверный URL: %1").arg(fullUrl));
        return QNetworkRequest();
    }

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setHeader(QNetworkRequest::UserAgentHeader, "DLP-Agent/1.0");
    request.setTransferTimeout(m_timeout);

    LOG_DEBUG(QString("Создан запрос: %1").arg(fullUrl));

    return request;
}

void NetworkManager::registerAgent(const QString &agentId, const QString &hostname,
                                   const QString &ipAddr, const QString &osInfo) {
    QJsonObject data;
    data["agent_id"] = agentId;
    data["hostname"] = hostname;

    QString finalIpAddr = ipAddr.isEmpty() ? "127.0.0.1" : ipAddr;
    data["ip_address"] = finalIpAddr;

    if (!osInfo.isEmpty()) {
        data["os_info"] = osInfo;
    }

    QNetworkRequest request = createRequest("/api/v1/agents/register");
    if (request.url().isEmpty()) {
        emit errorOccurred("Неверный URL сервера");
        return;
    }

    QNetworkReply* reply = m_manager->post(request, prepareJson(data));

    reply->setProperty("request_type", "register");
    reply->setProperty("agent_id", agentId);
    reply->setProperty("endpoint", "/api/v1/agents/register");

    m_activeReplies.append(reply);

    LOG_INFO(QString("Регистрация агента: %1").arg(agentId));
    LOG_DEBUG(QString("Данные: %1").arg(QString(prepareJson(data))));
}

void NetworkManager::sendHeartbeat(const QString &agentId) {
    QNetworkRequest request = createRequest(QString("/api/v1/agents/%1/heartbeat").arg(agentId));
    if (request.url().isEmpty()) {
        emit errorOccurred("Неверный URL сервера");
        return;
    }

    QNetworkReply* reply = m_manager->put(request, QByteArray());
    reply->setProperty("request_type", "heartbeat");
    reply->setProperty("agent_id", agentId);
    reply->setProperty("endpoint", QString("/api/v1/agents/%1/heartbeat").arg(agentId));
    m_activeReplies.append(reply);

    LOG_DEBUG(QString("Отправка heartbeat: %1").arg(agentId));
}

void NetworkManager::sendEvent(const QJsonObject& event) {
    QJsonObject validatedEvent = event;

    if (!validatedEvent.contains("agent_id")) {
        LOG_ERROR("Событие не содержит agent_id");
        emit errorOccurred("Событие не содержит обязательного поля agent_id");
        return;
    }

    if (!validatedEvent.contains("file_name")) {
        validatedEvent["file_name"] = "unknown";
    }

    if (!validatedEvent.contains("content_sample")) {
        validatedEvent["content_sample"] = "";
    }

    if (!validatedEvent.contains("event_type")) {
        validatedEvent["event_type"] = "created";
    }

    if (!validatedEvent.contains("is_violation")) {
        validatedEvent["is_violation"] = false;
    }

    if (!validatedEvent.contains("file_path")) {
        validatedEvent["file_path"] = "";
    }

    QNetworkRequest request = createRequest("/api/v1/events");
    if (request.url().isEmpty()) {
        emit errorOccurred("Неверный URL сервера");
        return;
    }

    QNetworkReply* reply = m_manager->post(request, prepareJson(validatedEvent));

    reply->setProperty("request_type", "event");
    reply->setProperty("agent_id", validatedEvent["agent_id"].toString());
    reply->setProperty("endpoint", "/api/v1/events");

    m_activeReplies.append(reply);

    QString fileName = validatedEvent.value("file_name").toString("unknown");
    LOG_INFO(QString("Отправка события: %1").arg(fileName));
    LOG_DEBUG(QString("Данные события: %1").arg(QString(prepareJson(validatedEvent))));
}

void NetworkManager::getPoliciesForAgent() {
    QNetworkRequest request = createRequest("/api/v1/policies/agent");
    if (request.url().isEmpty()) {
        emit errorOccurred("Неверный URL сервера");
        return;
    }

    QNetworkReply* reply = m_manager->get(request);

    reply->setProperty("request_type", "get_policies");
    reply->setProperty("endpoint", "/api/v1/policies/agent");

    m_activeReplies.append(reply);

    LOG_DEBUG("Запрос политик для агента");
}

void NetworkManager::handleError(const QString& context, const QString& error) {
    QString errorMsg = QString("%1: %2").arg(context).arg(error);
    LOG_ERROR(errorMsg);
    emit errorOccurred(errorMsg);
}

void NetworkManager::onReplyFinished(QNetworkReply* reply) {
    m_activeReplies.removeAll(reply);

    QString requestType = reply->property("request_type").toString();
    QString agentId = reply->property("agent_id").toString();
    QString endpoint = reply->property("endpoint").toString();

    LOG_DEBUG(QString("Ответ на запрос: %1 (тип: %2)").arg(endpoint).arg(requestType));

    if (reply->error() != QNetworkReply::NoError) {
        QString errorDetails = QString("%1 (код: %2)").arg(reply->errorString()).arg(reply->error());
        handleError(requestType, errorDetails);

        if (requestType == "heartbeat") {
            emit heartbeatSent(false);
        }

        reply->deleteLater();
        return;
    }

    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QByteArray responseData = reply->readAll();

    LOG_DEBUG(QString("HTTP статус: %1").arg(statusCode));
    LOG_DEBUG(QString("Размер ответа: %1 байт").arg(responseData.size()));

    if (statusCode < 200 || statusCode >= 300) {
        handleError(requestType, QString("HTTP %1").arg(statusCode));

        if (requestType == "heartbeat") {
            emit heartbeatSent(false);
        }

        reply->deleteLater();
        return;
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(responseData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        handleError(requestType, QString("Ошибка парсинга JSON: %1").arg(parseError.errorString()));
        reply->deleteLater();
        return;
    }

    // Обработка успешных ответов
    if (requestType == "register") {
        if (doc.isObject()) {
            QJsonObject response = doc.object();
            LOG_INFO(QString("Агент %1 успешно зарегистрирован").arg(agentId));
            LOG_DEBUG(QString("Ответ регистрации: %1").arg(QString(responseData)));
            emit agentRegistered(response);
        } else {
            LOG_ERROR("Некорректный ответ при регистрации");
            emit errorOccurred("Некорректный ответ сервера при регистрации");
        }
    }
    else if (requestType == "heartbeat") {
        LOG_DEBUG(QString("Heartbeat для %1 подтвержден").arg(agentId));
        emit heartbeatSent(true);
    }
    else if (requestType == "event") {
        if (doc.isObject()) {
            QJsonObject response = doc.object();
            LOG_INFO("Событие успешно доставлено");
            LOG_DEBUG(QString("Ответ события: %1").arg(QString(responseData)));
            emit eventSent(response);
        } else {
            LOG_ERROR("Некорректный ответ при отправке события");
            emit errorOccurred("Некорректный ответ сервера при отправке события");
        }
    }
    else if (requestType == "get_policies") {
        QJsonArray policies;

        if (doc.isArray()) {
            policies = doc.array();
            LOG_INFO(QString("Получено %1 политик").arg(policies.size()));
        } else if (doc.isObject() && doc.object().contains("policies")) {
            policies = doc.object()["policies"].toArray();
            LOG_INFO(QString("Получено %1 политик").arg(policies.size()));
        } else {
            LOG_WARNING("Формат ответа политик не соответствует ожидаемому");
        }

        emit policiesReceived(policies);
    }

    reply->deleteLater();
}