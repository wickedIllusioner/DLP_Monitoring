#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>

class LoginDialog : public QDialog {
    Q_OBJECT

public:
    explicit LoginDialog(QWidget *parent = nullptr);
    QString getUsername() const { return m_usernameEdit->text().trimmed(); }
    QString getPassword() const { return m_passwordEdit->text(); }

    private slots:
        void onLoginClicked();

private:
    void setupUI();
    bool attemptLogin(const QString &username, const QString &password);

    QLineEdit *m_usernameEdit;
    QLineEdit *m_passwordEdit;
    QPushButton *m_loginButton;
    QPushButton *m_cancelButton;
};

#endif