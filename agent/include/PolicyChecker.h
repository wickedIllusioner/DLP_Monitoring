#ifndef POLICYCHECKER_H
#define POLICYCHECKER_H

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>
#include <QStringList>

// Структура для хранения DLP-политики
struct DlpPolicy {
    int id;
    QString name;
    QString pattern;
    QString severity;

    bool isValid() const {
        return !name.isEmpty() && !pattern.isEmpty() && !severity.isEmpty();
    }
};

// Структура для результата проверки
struct PolicyMatch {
    QString policyName;
    QString policyPattern;
    QString severity;
    QString matchedContent;
    int startPosition;
    int endPosition;

    bool operator==(const PolicyMatch& other) const {
        return policyName == other.policyName &&
               policyPattern == other.policyPattern &&
               severity == other.severity &&
               matchedContent == other.matchedContent &&
               startPosition == other.startPosition &&
               endPosition == other.endPosition;
    }

    bool operator!=(const PolicyMatch& other) const {
        return !(*this == other);
    }
};

class PolicyChecker : public QObject
{
    Q_OBJECT

public:
    explicit PolicyChecker(QObject* parent = nullptr);
    ~PolicyChecker() = default;

    // Основные методы
    bool loadPolicies(const QJsonArray& policies);
    QList<PolicyMatch> checkContent(const QString& content, const QString& filePath = "");

    // Управление политиками
    void addPolicy(const DlpPolicy& policy);
    void removePolicy(int policyId);
    void clearPolicies();

    // Вспомогательные методы
    int policyCount() const { return m_policies.size(); }
    QList<DlpPolicy> allPolicies() const { return m_policies.values(); }

    // Настройки
    void setCaseSensitive(bool sensitive);
    void setMaxContentSize(int bytes);
    QString lastError() const { return m_lastError; }

signals:
    void policiesLoaded(int count);
    void policyAdded(const DlpPolicy& policy);
    void policyRemoved(int policyId);
    void contentChecked(const QString& filePath, const QList<PolicyMatch>& matches);

private:
    // Вспомогательные методы
    bool compilePattern(const QString& pattern, QRegularExpression& regex) const;
    QString extractSample(const QString& content, int maxLength = 1000) const;
    DlpPolicy parsePolicy(const QJsonObject& json) const;

    QHash<int, DlpPolicy> m_policies;
    QHash<int, QRegularExpression> m_compiledPatterns;

    bool m_caseSensitive;
    int m_maxContentSize;
    QString m_lastError;
};

#endif //POLICYCHECKER_H
