#ifndef AGENTCREATIONDIALOG_H
#define AGENTCREATIONDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>

class AgentCreationDialog : public QDialog {
    Q_OBJECT

public:
    explicit AgentCreationDialog(QWidget *parent = nullptr);

    QString getAgentName() const;
    QStringList getDirectories() const;
    QString getConfigPath() const;

    private slots:
        void onAddDirectory();
    void onRemoveDirectory();
    void onBrowseConfig();
    void onStartClicked();

private:
    void setupUI();

    QLineEdit *m_agentNameEdit;
    QTextEdit *m_directoriesEdit;
    QLineEdit *m_configPathEdit;
    QPushButton *m_startButton;
    QPushButton *m_cancelButton;
    QPushButton *m_addDirButton;
    QPushButton *m_removeDirButton;
    QPushButton *m_browseConfigButton;
};

#endif //AGENTCREATIONDIALOG_H
