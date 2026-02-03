#include "../include/LoginDialog.h"
#include <QFormLayout>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QEventLoop>
#include <QTimer>

LoginDialog::LoginDialog(QWidget *parent)
    : QDialog(parent) {
    setWindowTitle("Авторизация");
    setMinimumWidth(300);
    setupUI();
}

void LoginDialog::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QFormLayout *formLayout = new QFormLayout();

    m_usernameEdit = new QLineEdit(this);
    formLayout->addRow("Логин:", m_usernameEdit);

    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    formLayout->addRow("Пароль:", m_passwordEdit);

    mainLayout->addLayout(formLayout);

    QHBoxLayout *buttonLayout = new QHBoxLayout();

    m_loginButton = new QPushButton("Войти", this);
    m_loginButton->setDefault(true);
    m_cancelButton = new QPushButton("Отмена", this);

    buttonLayout->addStretch();
    buttonLayout->addWidget(m_loginButton);
    buttonLayout->addWidget(m_cancelButton);
    mainLayout->addLayout(buttonLayout);

    connect(m_loginButton, &QPushButton::clicked, this, &LoginDialog::onLoginClicked);
    connect(m_cancelButton, &QPushButton::clicked, this, &LoginDialog::reject);
}

bool LoginDialog::attemptLogin(const QString &username, const QString &password) {
    QNetworkAccessManager manager;
    QEventLoop loop;
    bool success = false;

    QUrl url("http://localhost:8080/api/v1/auth/login");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject json;
    json["username"] = username;
    json["password"] = password;

    QJsonDocument doc(json);
    QNetworkReply *reply = manager.post(request, doc.toJson());

    QTimer::singleShot(3000, &loop, [&]() {
        if (reply->isRunning()) {
            reply->abort();
        }
        loop.quit();
    });

    QObject::connect(reply, &QNetworkReply::finished, [&]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray data = reply->readAll();
            QJsonDocument response = QJsonDocument::fromJson(data);
            if (!response.isNull() && response.object()["success"].toBool()) {
                success = true;
            }
        }
        loop.quit();
    });

    loop.exec();
    reply->deleteLater();
    return success;
}

void LoginDialog::onLoginClicked() {
    QString username = m_usernameEdit->text().trimmed();
    QString password = m_passwordEdit->text();

    if (username.isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Введите логин и пароль");
        return;
    }

    m_loginButton->setEnabled(false);
    m_cancelButton->setEnabled(false);

    bool loginSuccess = attemptLogin(username, password);

    m_loginButton->setEnabled(true);
    m_cancelButton->setEnabled(true);

    if (loginSuccess) {
        accept();
    } else {
        QMessageBox::critical(this, "Ошибка", "Неверные данные или сервер недоступен");
        m_passwordEdit->clear();
        m_passwordEdit->setFocus();
    }
}