#include "../include/ContentAnalyzer.h"
#include "../include/Logger.h"
#include <QFile>
#include <QTextStream>
#include <QMimeDatabase>
#include <QMimeType>

ContentAnalyzer::ContentAnalyzer(QObject* parent)
    : QObject(parent)
    , m_maxFileSize(10 * 1024 * 1024) // 10MB
    , m_sampleSize(50000) // 50KB
    , m_analyzedCount(0)
    , m_totalBytesRead(0)
{
    LOG_DEBUG("ContentAnalyzer инициализирован");
}

bool ContentAnalyzer::analyzeFile(const QString& filePath, PolicyChecker* checker)
{
    QFileInfo fileInfo(filePath);

    if (!fileInfo.exists()) {
        LOG_ERROR(QString("Файл не существует: %1").arg(filePath));
        emit analysisError(filePath, "Файл не существует");
        return false;
    }

    if (fileInfo.size() > m_maxFileSize) {
        LOG_DEBUG(QString("Файл слишком большой для анализа: %1 (%2 байт)")
                 .arg(filePath).arg(fileInfo.size()));
        emit fileAnalyzed(filePath, false, QList<PolicyMatch>(), fileInfo.size());
        return true;
    }

    if (isBinaryFile(filePath)) {
        LOG_DEBUG(QString("Бинарный файл пропущен: %1").arg(filePath));
        emit fileAnalyzed(filePath, false, QList<PolicyMatch>(), fileInfo.size());
        return true;
    }

    QString content = readFileContent(filePath);
    if (content.isEmpty()) {
        LOG_WARNING(QString("Не удалось прочитать содержимое файла: %1").arg(filePath));
        emit analysisError(filePath, "Не удалось прочитать содержимое");
        return false;
    }

    LOG_DEBUG(QString("Прочитано %1 байт из файла: %2").arg(content.size()).arg(filePath));

    QList<PolicyMatch> matches;
    bool hasViolations = false;

    if (checker) {
        matches = checker->checkContent(content, filePath);
        hasViolations = !matches.isEmpty();
    }

    m_analyzedCount++;
    m_totalBytesRead += content.size();

    emit fileAnalyzed(filePath, hasViolations, matches, fileInfo.size());

    LOG_DEBUG(QString("Анализ завершен. Нарушений: %1").arg(matches.size()));
    return true;
}

QString ContentAnalyzer::readFileContent(const QString& filePath) const
{
    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        LOG_ERROR(QString("Не удалось открыть файл: %1 (%2)")
                 .arg(filePath).arg(file.errorString()));
        return QString();
    }

    QTextStream stream(&file);

    QString content;
    if (m_sampleSize > 0) {
        content = stream.read(m_sampleSize);
    } else {
        content = stream.readAll();
    }

    file.close();
    return content;
}

bool ContentAnalyzer::isBinaryFile(const QString& filePath) const
{
    QMimeDatabase mimeDb;
    QMimeType mimeType = mimeDb.mimeTypeForFile(filePath);
    QString mimeName = mimeType.name();

    if (mimeName.startsWith("text/") || mimeName == "application/json" ||
        mimeName == "application/xml") {
        return false;
    }

    QStringList textExtensions = getTextFileExtensions();
    QString extension = QFileInfo(filePath).suffix().toLower();

    if (textExtensions.contains(extension)) {
        return false;
    }

    QFile file(filePath);
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.read(1024);
        file.close();

        int nullCount = 0;
        for (char c : data) {
            if (c == '\0') {
                nullCount++;
            }
        }

        if (data.size() > 0 && (nullCount * 100 / data.size()) > 1) {
            return true;
        }
    }

    return true;
}

QStringList ContentAnalyzer::getTextFileExtensions() const
{
    return QStringList()
        << "txt" << "log" << "csv" << "json" << "xml" << "html" << "htm"
        << "js" << "css" << "cpp" << "h" << "py" << "java" << "cs"
        << "php" << "rb" << "go" << "rs" << "md" << "ini" << "conf"
        << "yaml" << "yml" << "sql" << "sh" << "bat" << "ps1";
}