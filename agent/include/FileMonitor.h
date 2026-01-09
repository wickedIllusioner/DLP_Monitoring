#ifndef FILESCANNER_H
#define FILESCANNER_H

#include <QObject>
#include <QFileSystemWatcher>
#include <QTimer>
#include <QSet>
#include <QRegularExpression>


class FileMonitor : public QObject {
    Q_OBJECT

public:
    explicit FileMonitor(QObject* parent = nullptr);
    ~FileMonitor();

    bool startMonitoring(const QStringList& directories, bool recursive = true);
    void stopMonitoring();
    bool isMonitoring() const { return m_monitoring; }

    void setExcludePatterns(const QStringList& patterns);
    void setCheckInterval(int msec);
    void setMaxFileSize(qint64 bytes);

    QStringList monitoredDirectories() const;
    int monitoredFilesCount() const;

signals:
    void fileCreated(const QString& filePath, qint64 size);
    void fileModified(const QString& filePath, qint64 size);
    void fileDeleted(const QString& filePath);
    void fileRenamed(const QString& oldPath, const QString& newPath);
    void fileAccessed(const QString& filePath);

    void monitoringStarted();
    void monitoringStopped();
    void errorOccurred(const QString& error);

private slots:
    void onDirectoryChanged(const QString& path);
    void onFileChanged(const QString& path);
    void performFullScan();

private:
    bool addDirectory(const QString& directory, bool recursive = true);
    void removeDirectory(const QString& directory);

    void addSubdirectoriesToWatcher(const QString& directory);

    bool shouldMonitorFile(const QString& filePath) const;
    bool isExcluded(const QString& filePath) const;

    QStringList getDirectoryFiles(const QString& directory, bool recursive = true) const;

    QFileSystemWatcher* m_watcher;
    QTimer* m_scanTimer;
    QSet<QString> m_monitoredDirs;
    QSet<QString> m_allMonitoredFiles;
    QHash<QString, QSet<QString>> m_dirFiles;
    QHash<QString, qint64> m_fileSizes;

    QStringList m_excludePatterns;
    QList<QRegularExpression> m_excludeRegex;

    QString m_baseDirectory;
    bool m_monitoring;
    bool m_recursive;
    qint64 m_maxFileSize;
    int m_checkInterval;
};

#endif //FILESCANNER_H