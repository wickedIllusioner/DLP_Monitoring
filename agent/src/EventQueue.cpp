#include "../include/EventQueue.h"
#include "../include/Logger.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>


EventQueue::EventQueue(QObject* parent)
    : QObject(parent) {
    LOG_DEBUG("EventQueue инициализирован");
}

EventQueue::~EventQueue() {
    LOG_DEBUG("EventQueue уничтожен");
}

void EventQueue::enqueue(const QJsonObject& event) {
    QMutexLocker locker(&m_mutex);

    m_queue.enqueue(event);
    int queueSize = m_queue.size();

    LOG_DEBUG(QString("Событие добавлено в очередь. Размер очереди: %1").arg(queueSize));

    emit eventAdded(queueSize);
}

QJsonObject EventQueue::dequeue() {
    QMutexLocker locker(&m_mutex);

    if (m_queue.isEmpty()) {
        return QJsonObject();
    }

    QJsonObject event = m_queue.dequeue();
    int queueSize = m_queue.size();

    LOG_DEBUG(QString("Событие извлечено из очереди. Осталось: %1").arg(queueSize));

    emit eventProcessed(queueSize);

    if (queueSize == 0) {
        emit queueEmpty();
    }

    return event;
}

bool EventQueue::isEmpty() const {
    QMutexLocker locker(&m_mutex);
    return m_queue.isEmpty();
}

int EventQueue::size() const {
    QMutexLocker locker(&m_mutex);
    return m_queue.size();
}

void EventQueue::clear() {
    QMutexLocker locker(&m_mutex);

    int clearedCount = m_queue.size();
    m_queue.clear();

    LOG_INFO(QString("Очередь очищена. Удалено событий: %1").arg(clearedCount));
}

bool EventQueue::saveToFile(const QString& filePath) {
    QMutexLocker locker(&m_mutex);

    if (m_queue.isEmpty()) {
        LOG_DEBUG("Нет событий для сохранения");
        return true;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        LOG_ERROR(QString("Не удалось открыть файл для сохранения: %1").arg(filePath));
        return false;
    }

    // Очередь в JSON массив
    QJsonArray eventsArray;
    for (const QJsonObject& event : m_queue) {
        eventsArray.append(event);
    }

    QJsonDocument doc(eventsArray);
    qint64 bytesWritten = file.write(doc.toJson());
    file.close();

    if (bytesWritten > 0) {
        LOG_INFO(QString("Очередь сохранена в файл: %1 (событий: %2)")
                .arg(filePath).arg(m_queue.size()));
        return true;
    }

    LOG_ERROR("Не удалось записать данные в файл");
    return false;
}

bool EventQueue::loadFromFile(const QString& filePath) {
    QMutexLocker locker(&m_mutex);

    if (!QFile::exists(filePath)) {
        LOG_DEBUG(QString("Файл очереди не существует: %1").arg(filePath));
        return true;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        LOG_ERROR(QString("Не удалось открыть файл очереди: %1").arg(filePath));
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        LOG_ERROR(QString("Ошибка парсинга файла очереди: %1").arg(parseError.errorString()));
        return false;
    }

    if (!doc.isArray()) {
        LOG_ERROR("Файл очереди не содержит JSON массив");
        return false;
    }

    m_queue.clear();
    QJsonArray eventsArray = doc.array();

    for (const QJsonValue& value : eventsArray) {
        if (value.isObject()) {
            m_queue.enqueue(value.toObject());
        }
    }

    LOG_INFO(QString("Очередь загружена из файла: %1 (событий: %2)")
            .arg(filePath).arg(m_queue.size()));

    return true;
}