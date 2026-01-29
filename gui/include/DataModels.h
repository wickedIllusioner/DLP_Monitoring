#ifndef DATAMODELS_H
#define DATAMODELS_H

#include <QString>
#include <QDateTime>
#include <QJsonObject>

// Структура для политики DLP
struct DlpPolicy {
    int id;
    QString name;
    QString description;
    QString pattern;
    QString severity;
    bool isActive;
    QDateTime createdAt;
    QDateTime updatedAt;

    // Конвертация из JSON
    static DlpPolicy fromJson(const QJsonObject &json);
    // Конвертация в JSON
    QJsonObject toJson() const;
};

// Структура для инцидента
struct Incident {
    int id;
    QString fileName;
    QString filePath;
    QString severity;
    QString policyName;
    QString agentHostname;
    QString matchedContent;
    QString status;
    QDateTime createdAt;
    QDateTime resolvedAt;
    QString resolvedBy;

    int eventId;
    QString agentId;
    QString policyId;

    static Incident fromJson(const QJsonObject &json);
};

// Структура для события
struct Event {
    int id;
    QString agentId;
    QString agentName;
    QString filePath;
    QString fileName;
    QString eventType;
    qint64 fileSize;
    QString contentSample;
    bool isViolation;
    QString violationType;
    QDateTime detectedAt;

    static Event fromJson(const QJsonObject &json);
};

// Структура для агента
struct Agent {
    int id;
    QString agentId;  // UUID
    QString hostname;
    QString ipAddress;
    QString osInfo;
    QDateTime lastSeen;
    QDateTime createdAt;
    bool isOnline;

    static Agent fromJson(const QJsonObject &json);
};

#endif //DATAMODELS_H