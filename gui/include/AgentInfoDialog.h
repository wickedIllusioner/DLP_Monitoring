#ifndef AGENTINFODIALOG_H
#define AGENTINFODIALOG_H

#include <QDialog>
#include <QProcess>

class AgentInfoDialog : public QDialog {
    Q_OBJECT

public:
    explicit AgentInfoDialog(const QString &agentName,
                           const QStringList &directories,
                           QProcess *agentProcess,
                           int serverAgentId,
                           QWidget *parent = nullptr);

signals:
    void deleteAgentRequested(int agentId);

private slots:
    void onStopAgent();

private:
    QString m_agentName;
    QProcess *m_agentProcess;
    int m_serverAgentId;
};

#endif //AGENTINFODIALOG_H
