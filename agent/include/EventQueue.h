#ifndef EVENTQUEUE_H
#define EVENTQUEUE_H

#include <QObject>
#include <QQueue>
#include <QMutex>
#include <QJsonObject>

class EventQueue : public QObject
{
    Q_OBJECT

public:
    explicit EventQueue(QObject* parent = nullptr);
    ~EventQueue();

    // Управление очередью
    void enqueue(const QJsonObject& event);
    QJsonObject dequeue();
    bool isEmpty() const;
    int size() const;

    // Сохранение/загрузка на диск
    bool saveToFile(const QString& filePath);
    bool loadFromFile(const QString& filePath);

    // Очистка
    void clear();

    signals:
        void eventAdded(int queueSize);
    void eventProcessed(int queueSize);
    void queueEmpty();

private:
    QQueue<QJsonObject> m_queue;
    mutable QMutex m_mutex;
};

#endif //EVENTQUEUE_H
