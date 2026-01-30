#include "../include/AgentInfoDialog.h"
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QTextEdit>
#include <QTimer>

AgentInfoDialog::AgentInfoDialog(const QString &agentName,
                                 const QStringList &directories,
                                 QProcess *agentProcess,
                                 int serverAgentId,
                                 QWidget *parent)
    : QDialog(parent)
    , m_agentName(agentName)
    , m_agentProcess(agentProcess)
    , m_serverAgentId(serverAgentId) {

    setWindowTitle("Информация об агенте");
    setMinimumSize(500, 200);

    QVBoxLayout* layout = new QVBoxLayout(this);

    QLabel* infoLabel = new QLabel(
        QString("<b>Агент:</b> %1<br>"
                "<b>Статус:</b> %2")
        .arg(agentName)
        .arg(agentProcess->state() == QProcess::Running ? "Запущен" : "Остановлен")
    );
    layout->addWidget(infoLabel);

    if (!directories.isEmpty()) {
        QLabel* dirsLabel = new QLabel("<b>Директории для мониторинга:</b>", this);
        layout->addWidget(dirsLabel);

        QTextEdit* dirsEdit = new QTextEdit(this);
        dirsEdit->setReadOnly(true);
        dirsEdit->setMaximumHeight(100);
        dirsEdit->setText(directories.join("\n"));
        layout->addWidget(dirsEdit);
    }

    QPushButton* stopButton = new QPushButton("Отключить агента", this);
    stopButton->setStyleSheet("background-color: #dc3545; color: white;");
    stopButton->setMinimumHeight(30);

    connect(stopButton, &QPushButton::clicked, this, &AgentInfoDialog::onStopAgent);
    layout->addWidget(stopButton, 0, Qt::AlignRight);
}

void AgentInfoDialog::onStopAgent() {
    if (QMessageBox::question(this, "Подтверждение",
        QString("Отключить агента '%1'?").arg(m_agentName),
        QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {

        if (m_agentProcess && m_agentProcess->state() == QProcess::Running) {
            m_agentProcess->terminate();
            QTimer::singleShot(3000, m_agentProcess, &QProcess::kill);
        }

        if (m_serverAgentId > 0) {
            emit deleteAgentRequested(m_serverAgentId);
        }
        accept();
    }
}