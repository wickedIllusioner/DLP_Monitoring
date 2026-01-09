#include "../include/PolicyChecker.h"
#include "../include/Logger.h"
#include <QJsonDocument>
#include <QFile>
#include <QTextStream>

PolicyChecker::PolicyChecker(QObject* parent)
    : QObject(parent)
    , m_caseSensitive(false)
    , m_maxContentSize(10 * 1024 * 1024)
{
    LOG_DEBUG("PolicyChecker инициализирован");
}

// Загрузка политик из JSON-массива
bool PolicyChecker::loadPolicies(const QJsonArray& policies)
{
    clearPolicies();

    if (policies.isEmpty()) {
        LOG_WARNING("Получен пустой список политик");
        m_lastError = "Пустой список политик";
        return false;
    }

    int loadedCount = 0;
    int failedCount = 0;

    for (const QJsonValue& policyValue : policies) {
        if (!policyValue.isObject()) {
            LOG_WARNING("Политика не является JSON объектом");
            failedCount++;
            continue;
        }

        DlpPolicy policy = parsePolicy(policyValue.toObject());

        if (!policy.isValid()) {
            LOG_WARNING("Некорректная структура политики");
            failedCount++;
            continue;
        }

        // Компилируем регулярное выражение
        QRegularExpression regex;
        if (!compilePattern(policy.pattern, regex)) {
            LOG_WARNING(QString("Не удалось скомпилировать паттерн для политики: %1").arg(policy.name));
            failedCount++;
            continue;
        }

        // Сохраняем политику
        m_policies[policy.id] = policy;
        m_compiledPatterns[policy.id] = regex;
        loadedCount++;

        LOG_DEBUG(QString("Загружена политика: %1 (ID: %2, Сложность: %3)")
                 .arg(policy.name).arg(policy.id).arg(policy.severity));
    }

    LOG_INFO(QString("Загружено политик: %1 (не удалось: %2)").arg(loadedCount).arg(failedCount));
    emit policiesLoaded(loadedCount);

    if (loadedCount > 0) {
        return true;
    } else {
        m_lastError = QString("Не удалось загрузить ни одной политики (ошибок: %1)").arg(failedCount);
        return false;
    }
}

// Основной метод проверки содержимого
QList<PolicyMatch> PolicyChecker::checkContent(const QString& content, const QString& filePath)
{
    QList<PolicyMatch> matches;

    if (content.isEmpty()) {
        LOG_DEBUG("Пустое содержимое для проверки");
        return matches;
    }

    if (m_policies.isEmpty()) {
        LOG_DEBUG("Нет политик для проверки");
        return matches;
    }

    // Ограничение размера проверяемого контента
    QString contentToCheck = content;
    if (contentToCheck.size() > m_maxContentSize) {
        contentToCheck = contentToCheck.left(m_maxContentSize);
        LOG_DEBUG(QString("Содержимое обрезано до %1 байт").arg(m_maxContentSize));
    }

    LOG_DEBUG(QString("Проверка содержимого (%1 байт), политик: %2")
             .arg(contentToCheck.size()).arg(m_policies.size()));

    for (auto it = m_policies.constBegin(); it != m_policies.constEnd(); ++it) {
        int policyId = it.key();
        const DlpPolicy& policy = it.value();
        const QRegularExpression& regex = m_compiledPatterns[policyId];

        // Поиск совпадений в тексте
        QRegularExpressionMatchIterator matchIterator = regex.globalMatch(contentToCheck);

        while (matchIterator.hasNext()) {
            QRegularExpressionMatch match = matchIterator.next();

            if (match.hasMatch()) {
                PolicyMatch policyMatch;
                policyMatch.policyName = policy.name;
                policyMatch.policyPattern = policy.pattern;
                policyMatch.severity = policy.severity;
                policyMatch.matchedContent = match.captured();
                policyMatch.startPosition = match.capturedStart();
                policyMatch.endPosition = match.capturedEnd();

                matches.append(policyMatch);

                LOG_DEBUG(QString("Найдено совпадение: %1 -> '%2'")
                         .arg(policy.name).arg(policyMatch.matchedContent));
            }
        }
    }

    if (!matches.isEmpty()) {
        LOG_WARNING(QString("Найдено %1 нарушений в %2").arg(matches.size())
                   .arg(filePath.isEmpty() ? "содержимом" : filePath));

        // Группировка по политикам
        QHash<QString, int> matchesByPolicy;
        for (const PolicyMatch& match : matches) {
            matchesByPolicy[match.policyName]++;
        }

        for (auto it = matchesByPolicy.constBegin(); it != matchesByPolicy.constEnd(); ++it) {
            LOG_WARNING(QString("  %1: %2 совпадений").arg(it.key()).arg(it.value()));
        }
    } else {
        LOG_DEBUG("Нарушений не обнаружено");
    }

    if (!filePath.isEmpty()) {
        emit contentChecked(filePath, matches);
    }

    return matches;
}

// Добавление одной политики
void PolicyChecker::addPolicy(const DlpPolicy& policy)
{
    if (!policy.isValid()) {
        LOG_ERROR("Попытка добавить некорректную политику");
        m_lastError = "Некорректная политика";
        return;
    }

    // Компилируем паттерн
    QRegularExpression regex;
    if (!compilePattern(policy.pattern, regex)) {
        LOG_ERROR(QString("Не удалось скомпилировать паттерн: %1").arg(policy.pattern));
        m_lastError = "Неверный паттерн регулярного выражения";
        return;
    }

    m_policies[policy.id] = policy;
    m_compiledPatterns[policy.id] = regex;

    LOG_INFO(QString("Добавлена политика: %1 (ID: %2)").arg(policy.name).arg(policy.id));
    emit policyAdded(policy);
}

// Удаление политики по id
void PolicyChecker::removePolicy(int policyId)
{
    if (m_policies.contains(policyId)) {
        QString policyName = m_policies[policyId].name;
        m_policies.remove(policyId);
        m_compiledPatterns.remove(policyId);

        LOG_INFO(QString("Удалена политика: %1 (ID: %2)").arg(policyName).arg(policyId));
        emit policyRemoved(policyId);
    }
}

// Очистка всех политик
void PolicyChecker::clearPolicies()
{
    int count = m_policies.size();
    m_policies.clear();
    m_compiledPatterns.clear();

    LOG_INFO(QString("Очищено %1 политик").arg(count));
}

// Установка чувствительности к регистру
void PolicyChecker::setCaseSensitive(bool sensitive)
{
    if (m_caseSensitive != sensitive) {
        m_caseSensitive = sensitive;

        // Перекомпилируем все паттерны с новыми настройками
        for (auto it = m_policies.begin(); it != m_policies.end(); ++it) {
            QRegularExpression regex;
            if (compilePattern(it.value().pattern, regex)) {
                m_compiledPatterns[it.key()] = regex;
            }
        }

        LOG_DEBUG(QString("Чувствительность к регистру: %1").arg(sensitive ? "да" : "нет"));
    }
}


// Установка ограничения размера проверяемого контента
void PolicyChecker::setMaxContentSize(int bytes)
{
    if (bytes > 0 && bytes != m_maxContentSize) {
        m_maxContentSize = bytes;
        LOG_DEBUG(QString("Макс. размер контента: %1 байт").arg(bytes));
    }
}


// Компиляция регулярного выражения с учетом настроек
bool PolicyChecker::compilePattern(const QString& pattern, QRegularExpression& regex) const
{
    QRegularExpression::PatternOptions options = QRegularExpression::UseUnicodePropertiesOption;

    if (!m_caseSensitive) {
        options |= QRegularExpression::CaseInsensitiveOption;
    }

    regex.setPattern(pattern);
    regex.setPatternOptions(options);

    if (!regex.isValid()) {
        LOG_ERROR(QString("Неверное регулярное выражение: %1 (%2)")
                 .arg(pattern).arg(regex.errorString()));
        return false;
    }

    return true;
}


// Извлечение образца текста заданной длины
QString PolicyChecker::extractSample(const QString& content, int maxLength) const
{
    if (content.length() <= maxLength) {
        return content;
    }

    return content.left(maxLength) + "...";
}


// Парсинг JSON в объект DlpPolicy
DlpPolicy PolicyChecker::parsePolicy(const QJsonObject& json) const
{
    DlpPolicy policy;

    if (json.contains("id") && json["id"].isDouble()) {
        policy.id = json["id"].toInt();
    } else {
        LOG_WARNING("Политика не содержит ID или некорректный ID");
        return policy; // Невалидная
    }

    if (json.contains("name") && json["name"].isString()) {
        policy.name = json["name"].toString();
    } else {
        LOG_WARNING("Политика не содержит имени");
        return policy;
    }

    if (json.contains("pattern") && json["pattern"].isString()) {
        policy.pattern = json["pattern"].toString();
    } else {
        LOG_WARNING("Политика не содержит паттерна");
        return policy;
    }

    if (json.contains("severity") && json["severity"].isString()) {
        policy.severity = json["severity"].toString();
    } else {
        policy.severity = "medium";
    }

    return policy;
}