#ifndef CONTENTANALYZER_H
#define CONTENTANALYZER_H

#include <QObject>
#include <QString>
#include <QFileInfo>
#include "PolicyChecker.h"

class ContentAnalyzer : public QObject
{
    Q_OBJECT

public:
    explicit ContentAnalyzer(QObject* parent = nullptr);
    ~ContentAnalyzer() = default;

    // Основной метод анализа файла
    bool analyzeFile(const QString& filePath, PolicyChecker* checker = nullptr);

    // Настройки
    void setMaxFileSize(qint64 bytes) { m_maxFileSize = bytes; }
    void setSampleSize(int bytes) { m_sampleSize = bytes; }

    // Статистика
    int analyzedFilesCount() const { return m_analyzedCount; }
    qint64 totalBytesRead() const { return m_totalBytesRead; }

    QString readFileContent(const QString& filePath) const;

signals:
    // Результаты анализа
    void fileAnalyzed(const QString& filePath, bool hasViolations,
                    const QList<PolicyMatch>& matches, qint64 size);
    void analysisError(const QString& filePath, const QString& error);

private:
    // Вспомогательные методы
    bool isBinaryFile(const QString& filePath) const;
    QStringList getTextFileExtensions() const;

    qint64 m_maxFileSize;
    int m_sampleSize;
    int m_analyzedCount;
    qint64 m_totalBytesRead;
};

#endif //CONTENTANALYZER_H
