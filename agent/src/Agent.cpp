#include "../include/Agent.h"
#include "../include/Logger.h"
#include <QTimer>
#include <QDateTime>
#include <QDir>
#include <QJsonDocument>
#include <QNetworkInterface>

Agent::Agent(QObject* parent)
    : QObject(parent)
    , m_heartbeatTimer(new QTimer(this))
    , m_config(ConfigManager::instance())
    , m_running(false)
{ LOG_DEBUG("Агент инициализирован"); }

Agent::~Agent() {
    stop();
    LOG_DEBUG("Агент уничтожен");
}

bool Agent::start() {
    if (m_running) {
        LOG_WARNING("Агент уже запущен");
        return false;
    }
    LOG_INFO("Запуск DLP агента...");

    if (!m_config.isLoaded()) {
        m_config.loadConfig();
    }

    connect(&m_network, &NetworkManager::policiesReceived, this, &Agent::onPoliciesReceived);
    connect(&m_network, &NetworkManager::heartbeatSent, this, &Agent::onHeartbeatSent);
    connect(&m_network, &NetworkManager::eventSent, this, &Agent::onEventSent);
    connect(&m_network, &NetworkManager::errorOccurred, this, &Agent::onNetworkError);

    connect(&m_monitor, &FileMonitor::fileCreated, this, &Agent::onFileCreated);
    connect(&m_monitor, &FileMonitor::fileModified, this, &Agent::onFileModified);
    connect(&m_monitor, &FileMonitor::fileDeleted, this, &Agent::onFileDeleted);
    connect(&m_analyzer, &ContentAnalyzer::fileAnalyzed, this, &Agent::onFileAnalyzed);


    m_monitor.setExcludePatterns(m_config.get("monitoring/exclude_patterns").toStringList());
    m_monitor.setMaxFileSize(m_config.get("agent/max_file_size").toLongLong());

    m_analyzer.setMaxFileSize(m_config.get("agent/max_file_size").toLongLong());
    m_analyzer.setSampleSize(50000);

    registerAgent();
    loadPolicies();

    QStringList dirs = m_config.get("monitoring/directories").toStringList();
    if (dirs.isEmpty()) {
        LOG_ERROR("Нет директорий для мониторинга");
        return false;
    }

    if (!m_monitor.startMonitoring(dirs, true)) {
        LOG_ERROR("Не удалось запустить мониторинг");
        return false;
    }

    int heartbeatInterval = m_config.get("server/heartbeat_interval", 60).toInt() * 1000;
    m_heartbeatTimer->setInterval(heartbeatInterval);
    connect(m_heartbeatTimer, &QTimer::timeout, this, &Agent::sendHeartbeat);
    m_heartbeatTimer->start();

    m_running = true;

    LOG_INFO("DLP агент успешно запущен");
    emit started();

    return true;
}

void Agent::stop() {
    if (!m_running) {
        return;
    }

    LOG_INFO("Остановка DLP агента...");

    m_heartbeatTimer->stop();
    m_monitor.stopMonitoring();
    m_running = false;

    QString agentId = m_config.agentId();
    if (!agentId.isEmpty()) {
        QUrl url(m_config.get("server/url").toString() +
                QString("/api/v1/agents/%1").arg(agentId));
        QNetworkRequest request(url);
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

        QNetworkReply* reply = m_network.getManager()->deleteResource(request);
        connect(reply, &QNetworkReply::finished, [reply, agentId]() {
            if (reply->error() == QNetworkReply::NoError) {
                LOG_INFO(QString("Агент %1 удален с сервера").arg(agentId));
            } else {
                LOG_DEBUG(QString("Не удалось удалить агента: %1").arg(reply->errorString()));
            }
            reply->deleteLater();
        });
    }

    LOG_INFO("DLP агент остановлен");
    emit stopped();
}

void Agent::registerAgent() {
    QString agentId = m_config.agentId();
    QString hostname = m_config.get("agent/hostname").toString();
    QString osInfo = m_config.getOsInfo();

    QString ipAddr;
    QList<QHostAddress> addresses = QNetworkInterface::allAddresses();
    for (const QHostAddress &address : addresses) {
        if (address.protocol() == QAbstractSocket::IPv4Protocol &&
            address != QHostAddress(QHostAddress::LocalHost)) {
            ipAddr = address.toString();
            break;
            }
    }
    if (ipAddr.isEmpty()) {
        ipAddr = "127.0.0.1";
    }

    LOG_INFO(QString("Регистрация агента: %1").arg(agentId));
    m_network.registerAgent(agentId, hostname, ipAddr, osInfo);
}

void Agent::loadPolicies() {
    LOG_INFO("Загрузка политик DLP...");
    m_network.getPoliciesForAgent();
}

void Agent::sendHeartbeat() {
    QString agentId = m_config.agentId();
    LOG_DEBUG(QString("Отправка heartbeat: %1").arg(agentId));
    m_network.sendHeartbeat(agentId);
}

void Agent::sendEvent(const QString& filePath, const QString& content, const QString& eventType,
                      bool isViolation, const QList<PolicyMatch>& matches) {
    QJsonObject event;

    event["agent_id"] = m_config.agentId();
    event["file_path"] = filePath;
    event["file_name"] = QFileInfo(filePath).fileName();
    event["event_type"] = eventType;
    event["file_size"] = static_cast<qint64>(QFileInfo(filePath).size());
    event["content_sample"] = content.left(1000);
    event["detected_at"] = QDateTime::currentDateTime().toString(Qt::ISODate) + "Z";
    event["is_violation"] = isViolation;

    if (eventType == "deleted") {
        event["is_violation"] = m_violationFiles.contains(filePath);
        if (event["is_violation"].isBool()) {
            event["violation_type"] = "data_loss";
            event["severity"] = "high";
        }
    } else {
        event["is_violation"] = isViolation;
        if (eventType == "modified" && m_violationFiles.contains(filePath)) {
            event["is_violation"] = true;
            event["severity"] = "high";
        }
    }

    if (event["is_violation"].toBool() && !matches.isEmpty()) {
        QStringList policyNames;
        QStringList severities;

        for (const PolicyMatch& match : matches) {
            policyNames.append(match.policyName);
            severities.append(match.severity);
        }

        event["violation_type"] = policyNames.join(", ");
        event["matched_policy"] = policyNames.first();
        event["severity"] = severities.contains("critical") ? "critical" :
                           severities.contains("high") ? "high" :
                           severities.contains("medium") ? "medium" : "low";
    }

    m_network.sendEvent(event);
}

void Agent::onFileCreated(const QString& filePath, qint64 size) {
    LOG_INFO(QString("Файл создан: %1 (%2 байт)").arg(filePath).arg(size));
    analyzeAndSendEvent(filePath, size, "created");
}

void Agent::onFileModified(const QString& filePath, qint64 size) {
    LOG_INFO(QString("Файл изменен: %1 (%2 байт)").arg(filePath).arg(size));
    analyzeAndSendEvent(filePath, size, "modified");
}

void Agent::onFileDeleted(const QString& filePath) {
    LOG_INFO(QString("Файл удален: %1").arg(filePath));

    m_fileEventTypes.remove(filePath);

    QString content = "";
    bool hadViolation = m_violationFiles.contains(filePath);

    sendEvent(filePath, content, "deleted", hadViolation, QList<PolicyMatch>());
    m_violationFiles.remove(filePath);
}

void Agent::onFileAnalyzed(const QString& filePath, bool hasViolations,
                          const QList<PolicyMatch>& matches, qint64 size) {
    QString content = m_analyzer.readFileContent(filePath);

    if (content.isEmpty()) {
        LOG_WARNING(QString("Не удалось прочитать файл для отправки: %1").arg(filePath));
        m_fileEventTypes.remove(filePath);
        return;
    }

    QString eventType = m_fileEventTypes.value(filePath, "created");

    if (hasViolations) {
        m_violationFiles.insert(filePath);
    } else if (eventType == "modified") {
        m_violationFiles.remove(filePath);
    }

    sendEvent(filePath, content, eventType, hasViolations, matches);
    m_fileEventTypes.remove(filePath);
}

void Agent::onPoliciesReceived(const QJsonArray& policies) {
    if (m_checker.loadPolicies(policies)) {
        LOG_INFO(QString("Политики DLP загружены: %1 шт").arg(policies.size()));

        // !!!
        QStringList dirs = m_config.get("monitoring/directories").toStringList();
        if (!dirs.isEmpty()) {
            analyzeExistingFiles(dirs);
        }

    } else {
        LOG_ERROR("Не удалось загрузить политики DLP");
    }
}

void Agent::onHeartbeatSent(bool success) {
    if (success) {
        LOG_DEBUG("Heartbeat подтвержден сервером");
    } else {
        LOG_WARNING("Не удалось отправить heartbeat");
    }
}

void Agent::onEventSent(const QJsonObject& resp) {
    LOG_INFO("Событие успешно отправлено на сервер");
    QByteArray jsonData = QJsonDocument(resp).toJson();
    LOG_DEBUG(QString("Ответ сервера: %1").arg(QString::fromUtf8(jsonData)));
}

void Agent::onNetworkError(const QString& error) {
    LOG_ERROR(QString("Ошибка сети: %1").arg(error));
    emit errorOccurred(error);
}


void Agent::analyzeAndSendEvent(const QString &filePath, qint64 size, const QString &eventType) {
    m_fileEventTypes[filePath] = eventType;

    if (!m_analyzer.analyzeFile(filePath, &m_checker)) {
        LOG_WARNING(QString("Не удалось проанализировать файл: %1").arg(filePath));

        QString content = m_analyzer.readFileContent(filePath);
        sendEvent(filePath, content, eventType, false, QList<PolicyMatch>());
        m_fileEventTypes.remove(filePath);
    }
}


// !!!
void Agent::analyzeExistingFiles(const QStringList& dirs) {
    for (const QString& dir : dirs) {
        QDir directory(dir);
        if (!directory.exists()) {
            continue;
        }

        // Рекурсивный поиск файлов
        QStringList files = getFilesRecursive(directory);

        for (const QString& filePath : files) {
            if (!shouldMonitorFile(filePath)) {
                continue;
            }
            LOG_DEBUG(QString("Анализ существующего файла: %1").arg(filePath));

            QString content = m_analyzer.readFileContent(filePath);
            if (!content.isEmpty()) {
                QList<PolicyMatch> matches = m_checker.checkContent(content, filePath);
                bool hasViolations = !matches.isEmpty();

                if (hasViolations) {
                    LOG_INFO(QString("Файл содержит чувствительную информацию: %1").arg(filePath));
                    m_violationFiles.insert(filePath);
                }
            }
        }
    }
    LOG_INFO(QString("Начальный анализ завершен. Файлов с нарушениями: %1")
             .arg(m_violationFiles.size()));
}


QStringList Agent::getFilesRecursive(const QDir& dir) {
    QStringList files;

    QStringList entries = dir.entryList(QDir::Files | QDir::NoDotAndDotDot);
    for (const QString& entry : entries) {
        files.append(dir.absoluteFilePath(entry));
    }

    QStringList subdirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString& subdir : subdirs) {
        QDir subdirectory(dir.absoluteFilePath(subdir));
        files.append(getFilesRecursive(subdirectory));
    }
    return files;
}


bool Agent::shouldMonitorFile(const QString& filePath) const {
    QFileInfo info(filePath);

    if (info.size() > m_config.get("agent/max_file_size").toLongLong()) {
        return false;
    }

    QString fileName = info.fileName();
    QStringList excludePatterns = m_config.get("monitoring/exclude_patterns").toStringList();

    for (const QString& pattern : excludePatterns) {
        QRegularExpression regex(QRegularExpression::wildcardToRegularExpression(pattern),
                                QRegularExpression::CaseInsensitiveOption);
        if (regex.match(fileName).hasMatch()) {
            return false;
        }
    }
    return true;
}