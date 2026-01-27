#include "../include/DataModels.h"
#include <QJsonObject>
#include <QJsonDocument>


DlpPolicy DlpPolicy::fromJson(const QJsonObject &json)
{
    DlpPolicy policy;

    policy.id = json["id"].toInt();
    policy.name = json["name"].toString();
    policy.description = json["description"].toString();
    policy.pattern = json["pattern"].toString();
    policy.severity = json["severity"].toString();
    policy.isActive = json["is_active"].toBool();

    policy.createdAt = QDateTime::fromString(json["created_at"].toString(), Qt::ISODate);
    policy.updatedAt = QDateTime::fromString(json["updated_at"].toString(), Qt::ISODate);

    return policy;
}

QJsonObject DlpPolicy::toJson() const
{
    QJsonObject json;

    json["id"] = id;
    json["name"] = name;
    json["description"] = description;
    json["pattern"] = pattern;
    json["severity"] = severity;
    json["is_active"] = isActive;
    json["created_at"] = createdAt.toString(Qt::ISODate);
    json["updated_at"] = updatedAt.toString(Qt::ISODate);

    return json;
}


Incident Incident::fromJson(const QJsonObject &json)
{
    Incident incident;

    incident.id = json["id"].toInt();
    incident.severity = json["severity"].toString();
    incident.status = json["status"].toString();
    incident.matchedContent = json["matched_content"].toString();
    incident.createdAt = QDateTime::fromString(json["created_at"].toString(), Qt::ISODate);

    if (json.contains("Event") && json["Event"].isObject()) {
        QJsonObject eventObj = json["Event"].toObject();
        incident.fileName = eventObj["file_name"].toString();
        incident.filePath = eventObj["file_path"].toString();

        if (eventObj.contains("Agent") && eventObj["Agent"].isObject()) {
            QJsonObject agentObj = eventObj["Agent"].toObject();
            incident.agentHostname = agentObj["hostname"].toString();
        }
    }

    if (json.contains("Policy") && json["Policy"].isObject()) {
        QJsonObject policyObj = json["Policy"].toObject();
        incident.policyName = policyObj["name"].toString();
    }

    return incident;
}


Event Event::fromJson(const QJsonObject &json)
{
    Event event;

    event.id = json["id"].toInt();
    event.agentId = json["agent_id"].toString();
    event.filePath = json["file_path"].toString();
    event.fileName = json["file_name"].toString();
    event.eventType = json["event_type"].toString();

    if (json.contains("file_size")) {
        event.fileSize = json["file_size"].toVariant().toLongLong();
    }

    event.contentSample = json["content_sample"].toString();

    if (json.contains("is_violation")) {
        event.isViolation = json["is_violation"].toBool();
    }

    if (json.contains("violation_type")) {
        event.violationType = json["violation_type"].toString();
    }

    if (!json["detected_at"].isNull()) {
        event.detectedAt = QDateTime::fromString(json["detected_at"].toString(), Qt::ISODate);
    }

    return event;
}


Agent Agent::fromJson(const QJsonObject &json)
{
    Agent agent;

    agent.id = json["id"].toInt();
    agent.agentId = json["agent_id"].toString();
    agent.hostname = json["hostname"].toString();
    agent.ipAddress = json["ip_address"].toString();
    agent.osInfo = json["os_info"].toString();

    agent.lastSeen = QDateTime::fromString(json["last_seen"].toString(), Qt::ISODate);
    agent.createdAt = QDateTime::fromString(json["created_at"].toString(), Qt::ISODate);

    QDateTime now = QDateTime::currentDateTime();
    agent.isOnline = agent.lastSeen.isValid() &&
                    agent.lastSeen.secsTo(now) < 600; // 10 минут

    return agent;
}