#include "../include/FileMonitor.h"
#include "../include/Logger.h"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QDateTime>


FileMonitor::FileMonitor(QObject *parent)
    : QObject(parent)
    , m_watcher(new QFileSystemWatcher(this))
    , m_scanTimer(new QTimer(this))
    , m_monitoring(false)
    , m_recursive(true)
    , m_checkInterval(1000)
{
    m_scanTimer->setInterval(30000);
    connect(m_scanTimer, &QTimer::timeout, this, &FileMonitor::performFullScan);

    connect(m_watcher, &QFileSystemWatcher::directoryChanged, this, &FileMonitor::onDirectoryChanged);
    connect(m_watcher, &QFileSystemWatcher::fileChanged, this, &FileMonitor::onFileChanged);
    LOG_DEBUG("FileMonitor инициализирован");
}


FileMonitor::~FileMonitor() {
    stopMonitoring();
    LOG_DEBUG("Мониторинг прекращен");
}


bool FileMonitor::startMonitoring(const QStringList &directories, bool recursive) {
    if (m_monitoring) {
        LOG_WARNING("Мониторинг уже запущен");
        return false;
    }

    m_baseDirectory = QDir(directories.first()).canonicalPath();
    m_recursive = recursive;

    m_dirFiles.clear();
    m_fileSizes.clear();
    m_allMonitoredFiles.clear();

    bool allAdded = true;
    for (const QString& dir : directories) {
        if (!addDirectory(dir, recursive)) {
            allAdded = false;
        }
    }

    if (m_monitoredDirs.isEmpty()) {
        LOG_ERROR("Нет доступных директорий для мониторинга");
        return false;
    }

    QSet<QString> initialFiles;
    for (const QString& dir : m_monitoredDirs) {
        QStringList files = getDirectoryFiles(dir, recursive);
        initialFiles.unite(QSet<QString>(files.begin(), files.end()));
    }

    for (const QString& filePath : initialFiles) {
        QFileInfo info(filePath);
        qint64 size = info.size();
        m_fileSizes[filePath] = size;
        m_allMonitoredFiles.insert(filePath);
    }

    m_monitoring = true;
    m_scanTimer->start();

    LOG_INFO(QString("Мониторинг запущен. Директорий: %1, Рекурсивно: %2")
             .arg(m_monitoredDirs.size()).arg(recursive ? "да" : "нет"));

    emit monitoringStarted();
    return allAdded;
}


void FileMonitor::stopMonitoring() {
    if (!m_monitoring) {
        return;
    }

    m_scanTimer->stop();

    QStringList dirs = m_watcher->directories();
    QStringList files = m_watcher->files();

    if (!dirs.isEmpty()) {
        m_watcher->removePaths(dirs);
    }

    if (!files.isEmpty()) {
        m_watcher->removePaths(files);
    }

    m_monitoredDirs.clear();
    m_dirFiles.clear();
    m_fileSizes.clear();
    m_monitoring = false;

    LOG_INFO("Мониторинг остановлен");
    emit monitoringStopped();
}


void FileMonitor::setExcludePatterns(const QStringList& patterns) {
    m_excludePatterns = patterns;
    m_excludeRegex.clear();

    for (const QString& pattern : patterns) {
        QString regexPattern = QRegularExpression::wildcardToRegularExpression(pattern);
        QRegularExpression regex(regexPattern, QRegularExpression::CaseInsensitiveOption);
        if (regex.isValid()) {
            m_excludeRegex.append(regex);
        } else {
            LOG_WARNING(QString("Некорректный паттерн исключения: %1").arg(pattern));
        }
    }

    LOG_DEBUG(QString("Установлено %1 паттернов исключения").arg(m_excludeRegex.size()));
}


void FileMonitor::setCheckInterval(int msec) {
    if (msec > 0) {
        m_checkInterval = msec;
        LOG_DEBUG(QString("Интервал проверки установлен: %1 мс").arg(m_checkInterval));
    }
}

void FileMonitor::setMaxFileSize(qint64 bytes) {
    m_maxFileSize = bytes;
    LOG_DEBUG(QString("Макс. размер файла: %1 байт").arg(bytes));
}


bool FileMonitor::addDirectory(const QString &directory, bool recursive) {
    QDir dir(directory);

    if (!dir.exists()) {
        LOG_WARNING(QString("Директория не существует: %1").arg(directory));
        emit errorOccurred(QString("Директория не существует: %1").arg(directory));
        return false;
    }

    QString canonPath = dir.canonicalPath();
    if (m_monitoredDirs.contains(canonPath)) {
        LOG_DEBUG(QString("Директория уже в мониторинге:").arg(directory));
        return false;
    }

    m_monitoredDirs.insert(canonPath);

    if (m_watcher->addPath(canonPath)) {
        LOG_DEBUG(QString("Директория добавлена в мониторинг: %1").arg(canonPath));

        if (recursive) { addSubdirectoriesToWatcher(canonPath); }
        return true;
    } else {
        m_monitoredDirs.remove(canonPath);
        LOG_ERROR(QString("Не удалось добавить директорию: %1").arg(canonPath));
        emit errorOccurred(QString("Не удалось добавить директорию: %1").arg(canonPath));
        return false;
    }
}


void FileMonitor::removeDirectory(const QString &directory) {
    QDir dir(directory);
    QString canonPath = dir.canonicalPath();

    if (m_watcher->removePath(canonPath)) {
        m_monitoredDirs.remove(canonPath);
        LOG_DEBUG(QString("Директория удалена из мониторинга: %1").arg(canonPath));
    }
}



void FileMonitor::addSubdirectoriesToWatcher(const QString &directory) {
    QDir dir(directory);
    if (!dir.exists()) return;

    QStringList subdirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString& subdir : subdirs) {
        QString subdirPath = dir.absoluteFilePath(subdir);
        QString canonPath = QDir(subdirPath).canonicalPath();

        if (!m_monitoredDirs.contains(canonPath)) {
            if (m_watcher->addPath(canonPath)) {
                m_monitoredDirs.insert(canonPath);
                LOG_DEBUG(QString("Поддиректория добавлена в мониторинг: %1").arg(canonPath));
            }
        }

        addSubdirectoriesToWatcher(canonPath);
    }
}


QStringList FileMonitor::getDirectoryFiles(const QString &directory, bool recursive) const {
    QStringList files;
    QDir dir(directory);

    if (!dir.exists()) {
        return files;
    }

    QStringList entries = dir.entryList(QDir::Files | QDir::NoDotAndDotDot);
    for (const QString& entry : entries) {
        QString filePath = dir.absoluteFilePath(entry);
        if (shouldMonitorFile(filePath)) { files.append(filePath); }
    }

    if (recursive) {
        QStringList subdirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString& subdir : subdirs) {
            QString subdirPath = dir.absoluteFilePath(subdir);
            files.append(getDirectoryFiles(subdirPath, recursive));
        }
    }

    return files;
}


bool FileMonitor::shouldMonitorFile(const QString &filePath) const {
    QFileInfo info(filePath);
    if (info.size() > m_maxFileSize) {
        return false;
    }

    QString fileName = info.fileName();
    if (fileName.contains("Untitled Document", Qt::CaseInsensitive)) {
        return false;
    }

    if (isExcluded(filePath)) {
        return false;
    }

    return true;
}


bool FileMonitor::isExcluded(const QString &filePath) const {
    QString fileName = QFileInfo(filePath).fileName();

    for (const QRegularExpression& regex : m_excludeRegex) {
        if (regex.match(fileName).hasMatch()) {
            LOG_DEBUG(QString("Файл исключен: %1 (паттерн: %2)")
                      .arg(fileName).arg(regex.pattern()));
            return true;
        }
    }

    return false;
}


void FileMonitor::onDirectoryChanged(const QString &path) {
    LOG_DEBUG(QString("Изменение в директории: %1").arg(path));

    static QTimer* debounceTimer = nullptr;
    if (!debounceTimer) {
        debounceTimer = new QTimer(this);
        debounceTimer->setSingleShot(true);
        debounceTimer->setInterval(m_checkInterval);
    }

    if (debounceTimer->isActive()) {
        debounceTimer->stop();
    }
    debounceTimer->start();

    connect(debounceTimer, &QTimer::timeout, this, [this, path]() {
        if (m_monitoring) {
            LOG_DEBUG(QString("Обработка изменения директории: %1").arg(path));

            QSet<QString> currentFiles;
            for (const QString& dir : m_monitoredDirs) {
                QStringList files = getDirectoryFiles(dir, m_recursive);
                currentFiles.unite(QSet<QString>(files.begin(), files.end()));
            }
            QSet<QString> oldFiles = m_allMonitoredFiles;

            // Новые файлы
            QSet<QString> createdFiles = currentFiles - oldFiles;
            for (const QString& filePath : createdFiles) {
                QFileInfo info(filePath);
                qint64 size = info.size();
                m_fileSizes[filePath] = size;
                m_allMonitoredFiles.insert(filePath);

                LOG_DEBUG(QString("Файл создан: %1 (%2 байт)").arg(filePath).arg(size));
                emit fileCreated(filePath, size);
            }

            // Удаленные файлы
            QSet<QString> deletedFiles = oldFiles - currentFiles;
            for (const QString& filePath : deletedFiles) {
                m_fileSizes.remove(filePath);
                m_allMonitoredFiles.remove(filePath);

                LOG_DEBUG(QString("Файл удален: %1").arg(filePath));
                emit fileDeleted(filePath);
            }

            // Модифицированные файлы
            QSet<QString> existingFiles = currentFiles & oldFiles;
            for (const QString& filePath : existingFiles) {
                QFileInfo info(filePath);
                qint64 newSize = info.size();
                qint64 oldSize = m_fileSizes.value(filePath, -1);

                if (newSize != oldSize && oldSize != -1) {
                    m_fileSizes[filePath] = newSize;

                    LOG_DEBUG(QString("Файл изменен: %1 (%2 -> %3 байт)")
                              .arg(filePath).arg(oldSize).arg(newSize));
                    emit fileModified(filePath, newSize);
                }
            }

            QStringList files = getDirectoryFiles(path, m_recursive);
            QSet<QString> fileSet(files.begin(), files.end());
            m_dirFiles[path] = fileSet;
        }
    });
}


void FileMonitor::onFileChanged(const QString& path) {
    LOG_DEBUG(QString("Файл изменен: %1").arg(path));

    if (m_allMonitoredFiles.contains(path)) {
        QFileInfo info(path);
        if (info.exists()) {
            qint64 newSize = info.size();
            qint64 oldSize = m_fileSizes.value(path, -1);

            if (newSize != oldSize) {
                m_fileSizes[path] = newSize;
                emit fileModified(path, newSize);
            }
        } else {
            m_allMonitoredFiles.remove(path);
            m_fileSizes.remove(path);
            emit fileDeleted(path);
        }
    }
}

void FileMonitor::performFullScan() {
    if (!m_monitoring) {
        return;
    }

    LOG_DEBUG("Выполнение полного сканирования...");

    QSet<QString> currentFiles;
    for (const QString& dir : m_monitoredDirs) {
        QStringList files = getDirectoryFiles(dir, m_recursive);
        currentFiles.unite(QSet<QString>(files.begin(), files.end()));
    }

    QSet<QString> oldFiles = m_allMonitoredFiles;

    // Новые файлы
    QSet<QString> createdFiles = currentFiles - oldFiles;
    for (const QString& filePath : createdFiles) {
        QFileInfo info(filePath);
        qint64 size = info.size();
        m_fileSizes[filePath] = size;
        m_allMonitoredFiles.insert(filePath);

        LOG_DEBUG(QString("Файл создан: %1 (%2 байт)").arg(filePath).arg(size));
        emit fileCreated(filePath, size);
    }

    // Удаленные файлы
    QSet<QString> deletedFiles = oldFiles - currentFiles;
    for (const QString& filePath : deletedFiles) {
        m_fileSizes.remove(filePath);
        m_allMonitoredFiles.remove(filePath);

        LOG_DEBUG(QString("Файл удален: %1").arg(filePath));
        emit fileDeleted(filePath);
    }

    // Модифицированные файлы
    QSet<QString> existingFiles = currentFiles & oldFiles;
    for (const QString& filePath : existingFiles) {
        QFileInfo info(filePath);
        qint64 newSize = info.size();
        qint64 oldSize = m_fileSizes.value(filePath, -1);

        if (newSize != oldSize && oldSize != -1) {
            m_fileSizes[filePath] = newSize;

            LOG_DEBUG(QString("Файл изменен: %1 (%2 -> %3 байт)")
                      .arg(filePath).arg(oldSize).arg(newSize));
            emit fileModified(filePath, newSize);
        }
    }

    m_dirFiles.clear();
    for (const QString& dir : m_monitoredDirs) {
        QStringList files = getDirectoryFiles(dir, m_recursive);
        QSet<QString> fileSet(files.begin(), files.end());
        m_dirFiles[dir] = fileSet;
    }

    LOG_DEBUG(QString("Полное сканирование завершено. Файлов в мониторинге: %1")
              .arg(m_allMonitoredFiles.size()));
}



QStringList FileMonitor::monitoredDirectories() const {
    return m_monitoredDirs.values();
}

int FileMonitor::monitoredFilesCount() const {
    return m_allMonitoredFiles.size();
}
