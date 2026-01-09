#include "../include/MainWindow.h"
#include "../include/Dialogs.h"
#include <QApplication>
#include <QMenuBar>
#include <QToolBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QMessageBox>
#include <QDateEdit>
#include <QMenu>
#include <QAction>
#include <QKeySequence>
#include <algorithm>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_apiClient(new ApiClient(this))
    , m_autoRefreshTimer(new QTimer(this))
{
    setupUI();
    setupConnections();

    onRefreshData();
    m_autoRefreshTimer->start(30000);
}


void MainWindow::setupUI()
{
    setWindowTitle("DLP Monitoring System");
    setMinimumSize(1024, 768);

    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    setupToolBar();
    setupTabs();
    mainLayout->addWidget(m_tabWidget);

    setupStatusBar();
}

void MainWindow::setupConnections()
{
    // API сигналы
    connect(m_apiClient, &ApiClient::policiesFetched, this, &MainWindow::onPoliciesFetched);
    connect(m_apiClient, &ApiClient::incidentsFetched, this, &MainWindow::onIncidentsFetched);
    connect(m_apiClient, &ApiClient::eventsFetched, this, &MainWindow::onEventsFetched);
    connect(m_apiClient, &ApiClient::agentsFetched, this, &MainWindow::onAgentsFetched);
    connect(m_apiClient, &ApiClient::statisticsFetched, this, &MainWindow::onStatisticsFetched);
    connect(m_apiClient, &ApiClient::errorOccurred, this, &MainWindow::onErrorOccurred);

    // Кнопки
    connect(m_btnAddPolicy, &QPushButton::clicked, this, &MainWindow::onAddPolicy);
    connect(m_btnEditPolicy, &QPushButton::clicked, this, &MainWindow::onEditPolicy);
    connect(m_btnDeletePolicy, &QPushButton::clicked, this, &MainWindow::onDeletePolicy);
    connect(m_btnRefresh, &QPushButton::clicked, this, &MainWindow::onRefreshData);
    connect(m_btnFilter, &QPushButton::clicked, this, &MainWindow::onFilterIncidents);
    connect(m_btnExport, &QPushButton::clicked, this, &MainWindow::onExportData);

    // Таймер автообновления
    connect(m_autoRefreshTimer, &QTimer::timeout, this, &MainWindow::onRefreshData);
}

void MainWindow::setupMenuBar()
{
    QMenuBar *menuBar = new QMenuBar(this);
    setMenuBar(menuBar);

    // Меню File
    QMenu *fileMenu = menuBar->addMenu("Файл");
    fileMenu->addAction("Экспорт...", this, &MainWindow::onExportData);
    fileMenu->addSeparator();
    fileMenu->addAction("Выход", qApp, &QApplication::quit);

    // Меню View
    QMenu *viewMenu = menuBar->addMenu("Вид");
    QAction *refreshAction = new QAction("Обновить", this);
    refreshAction->setShortcut(QKeySequence::Refresh);
    connect(refreshAction, &QAction::triggered, this, &MainWindow::onRefreshData);
    viewMenu->addAction(refreshAction);

    // Меню Settings
    QMenu *settingsMenu = menuBar->addMenu("Настройки");
    settingsMenu->addAction("Автообновление...");

    // Меню Help
    QMenu *helpMenu = menuBar->addMenu("Помощь");
    helpMenu->addAction("О программе");
}

void MainWindow::setupToolBar()
{
    QToolBar *toolBar = addToolBar("Панель инструментов");
    toolBar->setMovable(false);

    // Кнопка обновления
    m_btnRefresh = new QPushButton("Обновить", this);
    toolBar->addWidget(m_btnRefresh);

    toolBar->addSeparator();

    // Кнопка фильтра
    m_btnFilter = new QPushButton("Фильтр", this);
    toolBar->addWidget(m_btnFilter);

    // Кнопка экспорта
    m_btnExport = new QPushButton("Экспорт", this);
    toolBar->addWidget(m_btnExport);

    toolBar->addSeparator();

    // Статус подключения
    QLabel *statusLabel = new QLabel("Статус:", this);
    toolBar->addWidget(statusLabel);

    QLabel *connectionStatus = new QLabel("Подключено", this);
    connectionStatus->setStyleSheet("color: green; font-weight: bold;");
    toolBar->addWidget(connectionStatus);

    toolBar->addWidget(new QLabel(this));
}

void MainWindow::setupTabs()
{
    m_tabWidget = new QTabWidget(this);

    // ============ Вкладка инцидентов ============
    m_incidentsTab = new QWidget();
    QVBoxLayout *incidentsLayout = new QVBoxLayout(m_incidentsTab);

    // Панель фильтров
    QGroupBox *filterGroup = new QGroupBox("Фильтры", m_incidentsTab);
    QHBoxLayout *filterLayout = new QHBoxLayout(filterGroup);

    m_severityFilter = new QComboBox(filterGroup);
    m_severityFilter->addItems({"Все", "info", "low", "medium", "high", "critical"});

    m_statusFilter = new QComboBox(filterGroup);
    m_statusFilter->addItems({"Все", "new", "investigating", "resolved", "false_positive"});

    m_dateFromFilter = new QDateEdit(filterGroup);
    m_dateFromFilter->setCalendarPopup(true);
    m_dateFromFilter->setDate(QDate::currentDate().addDays(-7));

    m_dateToFilter = new QDateEdit(filterGroup);
    m_dateToFilter->setCalendarPopup(true);
    m_dateToFilter->setDate(QDate::currentDate());

    filterLayout->addWidget(new QLabel("Серьезность:"));
    filterLayout->addWidget(m_severityFilter);
    filterLayout->addWidget(new QLabel("Статус:"));
    filterLayout->addWidget(m_statusFilter);
    filterLayout->addWidget(new QLabel("С:"));
    filterLayout->addWidget(m_dateFromFilter);
    filterLayout->addWidget(new QLabel("По:"));
    filterLayout->addWidget(m_dateToFilter);

    incidentsLayout->addWidget(filterGroup);

    // Таблица инцидентов
    m_incidentsTable = new QTableView(m_incidentsTab);
    m_incidentsModel = new QStandardItemModel(0, 6, this);
    m_incidentsModel->setHorizontalHeaderLabels(
        QStringList() << "ID" << "Файл" << "Серьезность" << "Политика" << "Агент" << "Время");
    m_incidentsTable->setModel(m_incidentsModel);
    m_incidentsTable->horizontalHeader()->setStretchLastSection(true);

    incidentsLayout->addWidget(m_incidentsTable);

    m_tabWidget->addTab(m_incidentsTab, "Инциденты");

    // ============ Вкладка политик ============
    m_policiesTab = new QWidget();
    QVBoxLayout *policiesLayout = new QVBoxLayout(m_policiesTab);

    // Панель кнопок для политик
    QHBoxLayout *policyButtonsLayout = new QHBoxLayout();
    m_btnAddPolicy = new QPushButton("Добавить", m_policiesTab);
    m_btnEditPolicy = new QPushButton("Редактировать", m_policiesTab);
    m_btnDeletePolicy = new QPushButton("Удалить", m_policiesTab);

    policyButtonsLayout->addWidget(m_btnAddPolicy);
    policyButtonsLayout->addWidget(m_btnEditPolicy);
    policyButtonsLayout->addWidget(m_btnDeletePolicy);
    policyButtonsLayout->addStretch();

    policiesLayout->addLayout(policyButtonsLayout);

    // Таблица политик
    m_policiesTable = new QTableView(m_policiesTab);
    m_policiesModel = new QStandardItemModel(0, 5, this);
    m_policiesModel->setHorizontalHeaderLabels(
        QStringList() << "ID" << "Название" << "Паттерн" << "Серьезность" << "Активна");
    m_policiesTable->setModel(m_policiesModel);
    m_policiesTable->horizontalHeader()->setStretchLastSection(true);

    policiesLayout->addWidget(m_policiesTable);

    m_tabWidget->addTab(m_policiesTab, "Политики");

    // ============ Вкладка событий ============
    m_eventsTab = new QWidget();
    QVBoxLayout *eventsLayout = new QVBoxLayout(m_eventsTab);

    m_eventsTable = new QTableView(m_eventsTab);
    m_eventsModel = new QStandardItemModel(0, 6, this);
    m_eventsModel->setHorizontalHeaderLabels(
        QStringList() << "ID" << "Агент" << "Файл" << "Тип" << "Нарушение" << "Время");
    m_eventsTable->setModel(m_eventsModel);
    m_eventsTable->horizontalHeader()->setStretchLastSection(true);

    eventsLayout->addWidget(m_eventsTable);

    m_tabWidget->addTab(m_eventsTab, "События");

    // ============ Вкладка агентов ============
    m_agentsTab = new QWidget();
    QVBoxLayout *agentsLayout = new QVBoxLayout(m_agentsTab);

    m_agentsTable = new QTableView(m_agentsTab);
    m_agentsModel = new QStandardItemModel(0, 5, this);
    m_agentsModel->setHorizontalHeaderLabels(
        QStringList() << "ID" << "Имя хоста" << "IP" << "ОС" << "Последний контакт");
    m_agentsTable->setModel(m_agentsModel);
    m_agentsTable->horizontalHeader()->setStretchLastSection(true);

    agentsLayout->addWidget(m_agentsTable);

    m_tabWidget->addTab(m_agentsTab, "Агенты");

    // ============ Вкладка статистики ============
    m_statisticsTab = new QWidget();
    QVBoxLayout *statsLayout = new QVBoxLayout(m_statisticsTab);

    // Заглушка для статистики
    QLabel *statsLabel = new QLabel("Статистика будет отображаться здесь", m_statisticsTab);
    statsLabel->setAlignment(Qt::AlignCenter);
    statsLabel->setStyleSheet("font-size: 14pt; color: gray;");
    statsLayout->addWidget(statsLabel);

    m_tabWidget->addTab(m_statisticsTab, "Статистика");
}

void MainWindow::setupStatusBar()
{
    m_statusBar = statusBar();

    m_statusLabel = new QLabel("Готово", this);
    m_statusBar->addWidget(m_statusLabel);

    m_statsLabel = new QLabel("", this);
    m_statusBar->addPermanentWidget(m_statsLabel);
}

// ==================== Слоты для кнопок ====================

void MainWindow::onAddPolicy()
{
    PolicyDialog dialog(PolicyDialog::Create, this);

    if (dialog.exec() == QDialog::Accepted) {
        DlpPolicy policy = dialog.getPolicyData();
        m_apiClient->createPolicy(policy.toJson());

        m_statusLabel->setText("Создание политики...");
    }
}

void MainWindow::onEditPolicy()
{
    QModelIndex currentIndex = m_policiesTable->currentIndex();

    if (!currentIndex.isValid()) {
        QMessageBox::warning(this, "Внимание", "Выберите политику для редактирования");
        return;
    }

    int row = currentIndex.row();
    if (row >= 0 && row < m_policies.size()) {
        PolicyDialog dialog(PolicyDialog::Edit, this);
        dialog.setPolicyData(m_policies[row]);

        if (dialog.exec() == QDialog::Accepted) {
            DlpPolicy updatedPolicy = dialog.getPolicyData();
            m_apiClient->updatePolicy(updatedPolicy.id, updatedPolicy.toJson());

            m_statusLabel->setText("Обновление политики...");
        }
    }
}

void MainWindow::onDeletePolicy()
{
    QModelIndex currentIndex = m_policiesTable->currentIndex();

    if (!currentIndex.isValid()) {
        QMessageBox::warning(this, "Внимание", "Выберите политику для удаления");
        return;
    }

    int row = currentIndex.row();
    if (row >= 0 && row < m_policies.size()) {
        int id = m_policies[row].id;

        QMessageBox::StandardButton reply = QMessageBox::question(
            this, "Подтверждение",
            QString("Удалить политику '%1'?").arg(m_policies[row].name),
            QMessageBox::Yes | QMessageBox::No
        );

        if (reply == QMessageBox::Yes) {
            m_apiClient->deletePolicy(id);
            m_statusLabel->setText("Удаление политики...");
        }
    }
}

void MainWindow::onRefreshData()
{
    m_statusLabel->setText("Загрузка данных...");
    m_apiClient->fetchPolicies();
    m_apiClient->fetchIncidents();
    m_apiClient->fetchEvents();
    m_apiClient->fetchAgents();
    m_apiClient->fetchStatistics();
}

void MainWindow::onFilterIncidents()
{
    // Простая фильтрация по выбранным параметрам
    QString severity = m_severityFilter->currentText();
    QString status = m_statusFilter->currentText();
    QDate dateFrom = m_dateFromFilter->date();
    QDate dateTo = m_dateToFilter->date();

    m_statusLabel->setText("Применение фильтров...");
    m_apiClient->fetchIncidents(
        QDateTime(dateFrom, QTime(0, 0)),
        QDateTime(dateTo, QTime(23, 59, 59))
    );
}

void MainWindow::onExportData()
{
    QMessageBox::information(this, "Экспорт",
        "Функция экспорта будет реализована позже");
}

// ==================== Слоты ответов API ====================

void MainWindow::onPoliciesFetched(const QJsonArray &policies)
{
    m_policies.clear();
    m_policiesModel->removeRows(0, m_policiesModel->rowCount());

    for (const QJsonValue &value : policies) {
        DlpPolicy policy = DlpPolicy::fromJson(value.toObject());
        m_policies.append(policy);

        QList<QStandardItem*> items;
        items.append(new QStandardItem(QString::number(policy.id)));
        items.append(new QStandardItem(policy.name));
        items.append(new QStandardItem(policy.pattern));
        items.append(new QStandardItem(policy.severity));
        items.append(new QStandardItem(policy.isActive ? "Да" : "Нет"));

        m_policiesModel->appendRow(items);
    }

    updateStatusBar();
    m_statusLabel->setText(QString("Политики загружены: %1").arg(m_policies.size()));
}

void MainWindow::onIncidentsFetched(const QJsonArray &incidents)
{
    m_incidents.clear();
    m_incidentsModel->removeRows(0, m_incidentsModel->rowCount());

    for (const QJsonValue &value : incidents) {
        Incident incident = Incident::fromJson(value.toObject());
        m_incidents.append(incident);

        QList<QStandardItem*> items;
        items.append(new QStandardItem(QString::number(incident.id)));
        items.append(new QStandardItem(incident.fileName));

        // Серьезность с цветом
        QStandardItem *severityItem = new QStandardItem(incident.severity);
        if (incident.severity == "critical") severityItem->setBackground(Qt::red);
        else if (incident.severity == "high") severityItem->setBackground(QColor(255, 165, 0));
        else if (incident.severity == "medium") severityItem->setBackground(Qt::yellow);
        items.append(severityItem);

        items.append(new QStandardItem(incident.policyName));
        items.append(new QStandardItem(incident.agentHostname));
        items.append(new QStandardItem(incident.createdAt.toString("dd.MM.yyyy HH:mm")));

        m_incidentsModel->appendRow(items);
    }

    updateStatusBar();
}

void MainWindow::onEventsFetched(const QJsonArray &events)
{
    m_events.clear();
    m_eventsModel->removeRows(0, m_eventsModel->rowCount());

    for (const QJsonValue &value : events) {
        Event event = Event::fromJson(value.toObject());
        m_events.append(event);

        QList<QStandardItem*> items;
        items.append(new QStandardItem(QString::number(event.id)));
        items.append(new QStandardItem(event.agentId));
        items.append(new QStandardItem(event.fileName));
        items.append(new QStandardItem(event.eventType));
        items.append(new QStandardItem(event.isViolation ? "Да" : "Нет"));
        items.append(new QStandardItem(event.detectedAt.toString("dd.MM.yyyy HH:mm")));

        m_eventsModel->appendRow(items);
    }

    m_statusLabel->setText(QString("События загружены: %1").arg(m_events.size()));
}

void MainWindow::onAgentsFetched(const QJsonArray &agents)
{
    m_agents.clear();
    m_agentsModel->removeRows(0, m_agentsModel->rowCount());

    for (const QJsonValue &value : agents) {
        Agent agent = Agent::fromJson(value.toObject());
        m_agents.append(agent);

        QList<QStandardItem*> items;
        items.append(new QStandardItem(QString::number(agent.id)));
        items.append(new QStandardItem(agent.hostname));
        items.append(new QStandardItem(agent.ipAddress));
        items.append(new QStandardItem(agent.osInfo));

        QStandardItem *lastSeenItem = new QStandardItem(
            agent.lastSeen.toString("dd.MM.yyyy HH:mm:ss"));

        // Цвет для онлайн статуса
        if (agent.isOnline) {
            lastSeenItem->setForeground(Qt::darkGreen);
        } else {
            lastSeenItem->setForeground(Qt::gray);
        }

        items.append(lastSeenItem);
        m_agentsModel->appendRow(items);
    }

    m_statusLabel->setText(QString("Агенты загружены: %1 онлайн").arg(
        std::count_if(m_agents.begin(), m_agents.end(),
                      [](const Agent &a) { return a.isOnline; })));
}

void MainWindow::onStatisticsFetched(const QJsonObject &stats)
{
    updateStatusBar();
    m_statusLabel->setText("Статистика обновлена");
}

void MainWindow::onErrorOccurred(const QString &error)
{
    m_statusLabel->setText("Ошибка: " + error);
    QMessageBox::warning(this, "Ошибка", error);
}

// ==================== Слоты обновления интерфейса ====================

void MainWindow::updateStatusBar()
{
    int incidentsCount = m_incidents.size();
    int policiesCount = m_policies.size();
    int agentsCount = m_agents.size();

    QString statsText = QString("Инциденты: %1 | Политики: %2 | Агенты: %3")
                       .arg(incidentsCount).arg(policiesCount).arg(agentsCount);

    m_statsLabel->setText(statsText);
}