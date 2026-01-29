#include "../include/AgentCreationDialog.h"
#include <QListWidget>
#include <QGroupBox>

AgentCreationDialog::AgentCreationDialog(QWidget *parent)
    : QDialog(parent) {
    setWindowTitle("Создание нового агента");
    setMinimumSize(500, 400);
    setupUI();
}

void AgentCreationDialog::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Имя агента
    QGroupBox *nameGroup = new QGroupBox("Имя агента", this);
    QVBoxLayout *nameLayout = new QVBoxLayout(nameGroup);
    m_agentNameEdit = new QLineEdit(this);
    m_agentNameEdit->setPlaceholderText("Введите имя агента (например: Workstation-01)");
    nameLayout->addWidget(m_agentNameEdit);
    mainLayout->addWidget(nameGroup);

    // Директории для мониторинга
    QGroupBox *dirsGroup = new QGroupBox("Директории для мониторинга", this);
    QVBoxLayout *dirsLayout = new QVBoxLayout(dirsGroup);

    QHBoxLayout *dirButtonsLayout = new QHBoxLayout();
    m_addDirButton = new QPushButton("Добавить директорию", this);
    m_removeDirButton = new QPushButton("Удалить выбранную", this);
    dirButtonsLayout->addWidget(m_addDirButton);
    dirButtonsLayout->addWidget(m_removeDirButton);
    dirButtonsLayout->addStretch();

    m_directoriesEdit = new QTextEdit(this);
    m_directoriesEdit->setMaximumHeight(100);
    m_directoriesEdit->setPlaceholderText("Каждую директорию с новой строки");

    dirsLayout->addLayout(dirButtonsLayout);
    dirsLayout->addWidget(m_directoriesEdit);
    mainLayout->addWidget(dirsGroup);

    // Конфигурационный файл
    QGroupBox *configGroup = new QGroupBox("Конфигурационный файл (опционально)", this);
    QHBoxLayout *configLayout = new QHBoxLayout(configGroup);
    m_configPathEdit = new QLineEdit(this);
    m_configPathEdit->setPlaceholderText("Путь к конфигурационному файлу");
    m_browseConfigButton = new QPushButton("Обзор...", this);
    configLayout->addWidget(m_configPathEdit);
    configLayout->addWidget(m_browseConfigButton);
    mainLayout->addWidget(configGroup);

    // Кнопки
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    m_startButton = new QPushButton("Запуск агента", this);
    m_startButton->setDefault(true);
    m_cancelButton = new QPushButton("Отмена", this);

    buttonLayout->addStretch();
    buttonLayout->addWidget(m_startButton);
    buttonLayout->addWidget(m_cancelButton);
    mainLayout->addLayout(buttonLayout);

    connect(m_addDirButton, &QPushButton::clicked, this, &AgentCreationDialog::onAddDirectory);
    connect(m_removeDirButton, &QPushButton::clicked, this, &AgentCreationDialog::onRemoveDirectory);
    connect(m_browseConfigButton, &QPushButton::clicked, this, &AgentCreationDialog::onBrowseConfig);
    connect(m_startButton, &QPushButton::clicked, this, &AgentCreationDialog::onStartClicked);
    connect(m_cancelButton, &QPushButton::clicked, this, &AgentCreationDialog::reject);
}

void AgentCreationDialog::onAddDirectory() {
    QString dir = QFileDialog::getExistingDirectory(this, "Выберите директорию для мониторинга");
    if (!dir.isEmpty()) {
        QString currentText = m_directoriesEdit->toPlainText();
        if (!currentText.isEmpty()) {
            currentText += "\n";
        }
        m_directoriesEdit->setText(currentText + dir);
    }
}

void AgentCreationDialog::onRemoveDirectory() {
    QString currentText = m_directoriesEdit->toPlainText();
    QStringList dirs = currentText.split('\n', Qt::SkipEmptyParts);
    if (!dirs.isEmpty()) {
        dirs.removeLast();
        m_directoriesEdit->setText(dirs.join('\n'));
    }
}

void AgentCreationDialog::onBrowseConfig() {
    QString configFile = QFileDialog::getOpenFileName(this, "Выберите конфигурационный файл", "", "Config Files (*.conf *.ini)");
    if (!configFile.isEmpty()) {
        m_configPathEdit->setText(configFile);
    }
}

void AgentCreationDialog::onStartClicked() {
    QString agentName = m_agentNameEdit->text().trimmed();
    QString directoriesText = m_directoriesEdit->toPlainText().trimmed();

    if (agentName.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Введите имя агента");
        return;
    }

    if (directoriesText.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Укажите хотя бы одну директорию для мониторинга");
        return;
    }

    accept();
}

QString AgentCreationDialog::getAgentName() const {
    return m_agentNameEdit->text().trimmed();
}

QStringList AgentCreationDialog::getDirectories() const {
    return m_directoriesEdit->toPlainText().split('\n', Qt::SkipEmptyParts);
}

QString AgentCreationDialog::getConfigPath() const {
    return m_configPathEdit->text().trimmed();
}