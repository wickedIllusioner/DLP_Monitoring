#ifndef DIALOGS_H
#define DIALOGS_H

#include <QDialog>
#include <QLineEdit>
#include <QTextEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QMessageBox>
#include "DataModels.h"

class PolicyDialog : public QDialog {
    Q_OBJECT

public:
    enum Mode { Create, Edit };

    explicit PolicyDialog(Mode mode, QWidget *parent = nullptr);

    void setPolicyData(const DlpPolicy &policy);
    DlpPolicy getPolicyData() const;

    private slots:
        void onSaveClicked();
    void onTestPattern();

private:
    void setupUI();

    Mode m_mode;
    DlpPolicy m_policy;

    QLineEdit *m_nameEdit;
    QTextEdit *m_descriptionEdit;
    QTextEdit *m_patternEdit;
    QComboBox *m_severityCombo;
    QCheckBox *m_activeCheck;
    QPushButton *m_saveButton;
    QPushButton *m_cancelButton;
    QPushButton *m_testButton;
};

#endif //DIALOGS_H
