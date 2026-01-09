#include "../include/Agent.h"
#include "../include/Logger.h"
#include <QTimer>
#include <QDateTime>
#include <QJsonDocument>

Agent::Agent(QObject* parent)
    : QObject(parent)
    , m_heartbeatTimer(new QTimer(this))
    , m_config(ConfigManager::instance())
    , m_running(false)
{
    LOG_DEBUG("Агент инициализирован");
}

Agent::~Agent()
{
    stop();
    LOG_DEBUG("Агент уничтожен");
}

bool Agent::start()
{
    if (m_running) {
        LOG_WARNING("Агент уже запущен");
        return false;
    }

    LOG_INFO("Запуск DLP агента...");

    if (!m_config.isLoaded()) {
        m_config.loadConfig();
    }

    connect(&m_network, &NetworkManager::policiesReceived,
            this, &Agent::onPoliciesReceived);
    connect(&m_network, &NetworkManager::heartbeatSent,
            this, &Agent::onHeartbeatSent);
    connect(&m_network, &NetworkManager::eventSent,
            this, &Agent::onEventSent);
    connect(&m_network, &NetworkManager::errorOccurred,
            this, &Agent::onNetworkError);

    connect(&m_monitor, &FileMonitor::fileCreated,
            this, &Agent::onFileCreated);
    connect(&m_monitor, &FileMonitor::fileModified,
            this, &Agent::onFileModified);

    connect(&m_analyzer, &ContentAnalyzer::fileAnalyzed,
            this, &Agent::onFileAnalyzed);

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

    int heartbeatInterval = m_config.get("server/heartbeat_interval", 300).toInt() * 1000;
    m_heartbeatTimer->setInterval(heartbeatInterval);
    connect(m_heartbeatTimer, &QTimer::timeout, this, &Agent::sendHeartbeat);
    m_heartbeatTimer->start();

    m_running = true;

    LOG_INFO("DLP агент успешно запущен");
    emit started();

    return true;
}

void Agent::stop()
{
    if (!m_running) {
        return;
    }

    LOG_INFO("Остановка DLP агента...");

    m_heartbeatTimer->stop();
    m_monitor.stopMonitoring();
    m_running = false;

    LOG_INFO("DLP агент остановлен");
    emit stopped();
}

void Agent::registerAgent()
{
    QString agentId = m_config.agentId();
    QString hostname = m_config.get("agent/hostname").toString();

    LOG_INFO(QString("Регистрация агента: %1").arg(agentId));
    m_network.registerAgent(agentId, hostname);
}

void Agent::loadPolicies()
{
    LOG_INFO("Загрузка политик DLP...");
    m_network.getPoliciesForAgent();
}

void Agent::sendHeartbeat()
{
    QString agentId = m_config.agentId();
    LOG_DEBUG(QString("Отправка heartbeat: %1").arg(agentId));
    m_network.sendHeartbeat(agentId);
}

void Agent::sendEvent(const QString& filePath, const QString& content,
                      bool isViolation, const QList<PolicyMatch>& matches)
{
    QJsonObject event;

    event["agent_id"] = m_config.agentId();
    event["file_path"] = filePath;
    event["file_name"] = QFileInfo(filePath).fileName();
    event["event_type"] = "created";
    event["file_size"] = static_cast<qint64>(QFileInfo(filePath).size());
    event["content_sample"] = content.left(1000);
    event["detected_at"] = QDateTime::currentDateTime().toString(Qt::ISODate) + "Z";
    event["is_violation"] = isViolation;

    if (isViolation && !matches.isEmpty()) {
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

void Agent::onFileCreated(const QString& filePath, qint64 size)
{
    LOG_INFO(QString("Файл создан: %1 (%2 байт)").arg(filePath).arg(size));

    if (!m_analyzer.analyzeFile(filePath, &m_checker)) {
        LOG_WARNING(QString("Не удалось проанализировать файл: %1").arg(filePath));
    }
}

void Agent::onFileModified(const QString& filePath, qint64 size)
{
    LOG_INFO(QString("Файл изменен: %1 (%2 байт)").arg(filePath).arg(size));

    if (!m_analyzer.analyzeFile(filePath, &m_checker)) {
        LOG_WARNING(QString("Не удалось проанализировать файл: %1").arg(filePath));
    }
}

void Agent::onFileAnalyzed(const QString& filePath, bool hasViolations,
                          const QList<PolicyMatch>& matches, qint64 size)
{
    QString content = m_analyzer.readFileContent(filePath);

    if (content.isEmpty()) {
        LOG_WARNING(QString("Не удалось прочитать файл для отправки: %1").arg(filePath));
        return;
    }

    sendEvent(filePath, content, hasViolations, matches);
}

void Agent::onPoliciesReceived(const QJsonArray& policies)
{
    if (m_checker.loadPolicies(policies)) {
        LOG_INFO(QString("Политики DLP загружены: %1 шт").arg(policies.size()));
    } else {
        LOG_ERROR("Не удалось загрузить политики DLP");
    }
}

void Agent::onHeartbeatSent(bool success)
{
    if (success) {
        LOG_DEBUG("Heartbeat подтвержден сервером");
    } else {
        LOG_WARNING("Не удалось отправить heartbeat");
    }
}

void Agent::onEventSent(const QJsonObject& resp)
{
    LOG_INFO("Событие успешно отправлено на сервер");
    QByteArray jsonData = QJsonDocument(resp).toJson();
    LOG_DEBUG(QString("Ответ сервера: %1").arg(QString::fromUtf8(jsonData)));
}

void Agent::onNetworkError(const QString& error)
{
    LOG_ERROR(QString("Ошибка сети: %1").arg(error));
    emit errorOccurred(error);
}