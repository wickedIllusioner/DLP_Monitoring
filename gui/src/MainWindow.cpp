#include "../include/MainWindow.h"
#include "../include/Dialogs.h"
#include "../include/AgentCreationDialog.h"
#include "../include/AgentInfoDialog.h"
#include "../widgets/include/StatisticsTabWidget.h"

#include <QApplication>
#include <QSortFilterProxyModel>
#include <QToolBar>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QMessageBox>
#include <QInputDialog>
#include <QProcess>
#include <QSettings>
#include <algorithm>

using namespace std;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_apiClient(new ApiClient(this))
    , m_autoRefreshTimer(new QTimer(this)) {

    setupUI();
    setupConnections();
    onRefreshData();
    m_autoRefreshTimer->start(30000);
}


void MainWindow::setupUI() {
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

void MainWindow::setupConnections() {
    // API сигналы
    connect(m_apiClient, &ApiClient::policiesFetched, this, &MainWindow::onPoliciesFetched);
    connect(m_apiClient, &ApiClient::incidentsFetched, this, &MainWindow::onIncidentsFetched);
    connect(m_apiClient, &ApiClient::incidentStatusUpdated, this, &MainWindow::onIncidentStatusUpdated);
    connect(m_apiClient, &ApiClient::eventsFetched, this, &MainWindow::onEventsFetched);
    connect(m_apiClient, &ApiClient::agentsFetched, this, &MainWindow::onAgentsFetched);
    connect(m_apiClient, &ApiClient::statisticsFetched, this, &MainWindow::onStatisticsFetched);
    connect(m_apiClient, &ApiClient::errorOccurred, this, &MainWindow::onErrorOccurred);
    connect(m_apiClient, &ApiClient::policyCreated, this, &MainWindow::onPolicyCreated);
    connect(m_apiClient, &ApiClient::policyUpdated, this, &MainWindow::onPolicyUpdated);
    connect(m_apiClient, &ApiClient::policyDeleted, this, &MainWindow::onPolicyDeleted);

    // Кнопки
    connect(m_btnAddPolicy, &QPushButton::clicked, this, &MainWindow::onAddPolicy);
    connect(m_btnEditPolicy, &QPushButton::clicked, this, &MainWindow::onEditPolicy);
    connect(m_btnDeletePolicy, &QPushButton::clicked, this, &MainWindow::onDeletePolicy);
    connect(m_btnRefresh, &QPushButton::clicked, this, &MainWindow::onRefreshData);
    connect(m_btnChangeStatus, &QPushButton::clicked, this, &MainWindow::onChangeIncidentStatus);

    // Таймер автообновления
    connect(m_autoRefreshTimer, &QTimer::timeout, this, &MainWindow::onRefreshData);

    connect(m_severityFilter, &QComboBox::currentTextChanged, this, &MainWindow::onFilterChanged);
    connect(m_statusFilter, &QComboBox::currentTextChanged, this, &MainWindow::onFilterChanged);
}


void MainWindow::setupToolBar() {
    QToolBar *toolBar = addToolBar("Панель инструментов");
    toolBar->setMovable(false);

    m_btnRefresh = new QPushButton("Обновить", this);
    toolBar->addWidget(m_btnRefresh);
    toolBar->addSeparator();

    QLabel *statusLabel = new QLabel("Статус:", this);
    toolBar->addWidget(statusLabel);

    QLabel *connectionStatus = new QLabel("Подключено", this);
    connectionStatus->setStyleSheet("color: green; font-weight: bold;");
    toolBar->addWidget(connectionStatus);

    toolBar->addWidget(new QLabel(this));
}

void MainWindow::setupTabs() {
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

    filterLayout->addWidget(new QLabel("Серьезность:"));
    filterLayout->addWidget(m_severityFilter);
    filterLayout->addWidget(new QLabel("Статус:"));
    filterLayout->addWidget(m_statusFilter);

    incidentsLayout->addWidget(filterGroup);

    // Изменение статуса инцидента
    QHBoxLayout *incidentsButtonsLayout = new QHBoxLayout();
    m_btnChangeStatus = new QPushButton("Изменить статус", m_incidentsTab);
    incidentsButtonsLayout->addWidget(m_btnChangeStatus);
    incidentsButtonsLayout->addStretch();
    incidentsLayout->addLayout(incidentsButtonsLayout);

    // Таблица инцидентов
    m_incidentsTable = new QTableView(m_incidentsTab);
    m_incidentsModel = new QStandardItemModel(0, 6, this);
    m_incidentsModel->setHorizontalHeaderLabels(
        QStringList() << "Файл" << "Путь" << "Серьезность" << "Статус" << "Политика" << "Агент" << "Время");

    m_incidentsProxyModel = new QSortFilterProxyModel(this);
    m_incidentsProxyModel->setSourceModel(m_incidentsModel);
    m_incidentsProxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_incidentsTable->setModel(m_incidentsProxyModel);

    m_incidentsTable->horizontalHeader()->setStretchLastSection(false);
    m_incidentsTable->setColumnWidth(0, 150);  // Файл
    m_incidentsTable->setColumnWidth(1, 400);  // Путь
    m_incidentsTable->setColumnWidth(2, 100);  // Серьезность
    m_incidentsTable->setColumnWidth(3, 150);  // Политика
    m_incidentsTable->setColumnWidth(4, 250);  // Агент
    m_incidentsTable->setColumnWidth(5, 150);  // Время
    m_incidentsTable->setWordWrap(true);

    m_incidentsTable->setSortingEnabled(true);
    m_incidentsTable->horizontalHeader()->setStretchLastSection(true);
    m_incidentsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    m_incidentsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_incidentsTable->setSelectionMode(QAbstractItemView::SingleSelection);

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
    m_policiesModel = new QStandardItemModel(0, 4, this);
    m_policiesModel->setHorizontalHeaderLabels(
        QStringList() << "Название" << "Паттерн" << "Серьезность" << "Активна");
    m_policiesTable->setModel(m_policiesModel);
    m_policiesTable->horizontalHeader()->setStretchLastSection(true);
    m_policiesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    m_policiesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_policiesTable->setSelectionMode(QAbstractItemView::SingleSelection);

    // Редактирование по двойному клику
    connect(m_policiesTable, &QTableView::doubleClicked, [this](const QModelIndex &index) {
        if (index.isValid()) {
            onEditPolicy();
        }
    });

    policiesLayout->addWidget(m_policiesTable);
    m_tabWidget->addTab(m_policiesTab, "Политики");


    // ============ Вкладка событий ============
    m_eventsTab = new QWidget();
    QVBoxLayout *eventsLayout = new QVBoxLayout(m_eventsTab);

    m_eventsTable = new QTableView(m_eventsTab);
    m_eventsModel = new QStandardItemModel(0, 5, this);
    m_eventsModel->setHorizontalHeaderLabels(
        QStringList() << "Агент" << "Файл" << "Тип" << "Нарушение" << "Время");
    m_eventsTable->setModel(m_eventsModel);
    m_eventsTable->horizontalHeader()->setStretchLastSection(true);
    m_eventsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    m_eventsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_eventsTable->setSelectionMode(QAbstractItemView::SingleSelection);

    eventsLayout->addWidget(m_eventsTable);
    m_tabWidget->addTab(m_eventsTab, "События");


    // ============ Вкладка агентов ============
    m_agentsTab = new QWidget();
    QVBoxLayout *agentsLayout = new QVBoxLayout(m_agentsTab);

    // Панель кнопок для агентов
    QHBoxLayout *agentButtonsLayout = new QHBoxLayout();
    m_btnAddAgent = new QPushButton("Создание нового агента", m_agentsTab);
    agentButtonsLayout->addWidget(m_btnAddAgent);
    agentButtonsLayout->addStretch();
    agentsLayout->addLayout(agentButtonsLayout);

    m_agentsTable = new QTableView(m_agentsTab);
    m_agentsModel = new QStandardItemModel(0, 4, this);
    m_agentsModel->setHorizontalHeaderLabels(
        QStringList()  << "Имя агента" << "IP" << "ОС" << "Последний контакт");
    m_agentsTable->setModel(m_agentsModel);
    m_agentsTable->horizontalHeader()->setStretchLastSection(true);
    m_agentsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    m_agentsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_agentsTable->setSelectionMode(QAbstractItemView::SingleSelection);

    agentsLayout->addWidget(m_agentsTable);
    m_tabWidget->addTab(m_agentsTab, "Агенты");
    connect(m_btnAddAgent, &QPushButton::clicked, this, &MainWindow::onAddAgent);
    connect(m_agentsTable, &QTableView::doubleClicked, this, &MainWindow::onAgentDoubleClicked);


    // ============ Вкладка статистики ============
    m_statisticsTab = new QWidget();
    QVBoxLayout *statsLayout = new QVBoxLayout(m_statisticsTab);

    StatisticsTabWidget *statsWidget = new StatisticsTabWidget(m_statisticsTab);
    statsLayout->addWidget(statsWidget);

    m_tabWidget->addTab(m_statisticsTab, "Статистика");
}

void MainWindow::setupStatusBar() {
    m_statusBar = statusBar();

    m_statusLabel = new QLabel("Готово", this);
    m_statusBar->addWidget(m_statusLabel);

    m_statsLabel = new QLabel("", this);
    m_statusBar->addPermanentWidget(m_statsLabel);
}

// ==================== Слоты для кнопок ====================

void MainWindow::onAddPolicy() {
    PolicyDialog dialog(PolicyDialog::Create, this);

    if (dialog.exec() == QDialog::Accepted) {
        DlpPolicy policy = dialog.getPolicyData();
        m_apiClient->createPolicy(policy.toJson());

        m_statusLabel->setText("Создание политики...");
    }
}

void MainWindow::onEditPolicy() {
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

void MainWindow::onDeletePolicy() {
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

void MainWindow::onRefreshData() {
    m_statusLabel->setText("Загрузка данных...");
    m_apiClient->fetchPolicies();
    m_apiClient->fetchIncidents();
    m_apiClient->fetchEvents();
    m_apiClient->fetchAgents();
    m_apiClient->fetchStatistics();
}

void MainWindow::onChangeIncidentStatus() {
    QModelIndex currentIndex = m_incidentsTable->currentIndex();

    if (!currentIndex.isValid()) {
        QMessageBox::warning(this, "Внимание", "Выберите инцидент для изменения статуса");
        return;
    }

    int row = currentIndex.row();
    if (row >= 0 && row < m_incidents.size()) {
        Incident incident = m_incidents[row];

        QStringList statusList = {"new", "investigating", "resolved", "false_positive"};
        QString newStatus = QInputDialog::getItem(this,
            QString("Изменение статуса инцидента #%1").arg(incident.id),
            "Выберите новый статус:", statusList, statusList.indexOf(incident.status), false);

        if (!newStatus.isEmpty() && newStatus != incident.status) {
            QString resolvedBy;
            if (newStatus == "resolved") {
                resolvedBy = QInputDialog::getText(this, "Кем разрешено",
                    "Введите имя/идентификатор пользователя:");

                if (resolvedBy.isEmpty()) {
                    QMessageBox::warning(this, "Ошибка",
                        "Для статуса 'resolved' необходимо указать, кем разрешено");
                    return;
                }
            }

            m_apiClient->updateIncidentStatus(incident.id, newStatus, resolvedBy);
            m_statusLabel->setText(QString("Изменение статуса инцидента #%1...").arg(incident.id));
        }
    }
}

void MainWindow::onAddAgent() {
    AgentCreationDialog dialog(this);

    if (dialog.exec() == QDialog::Accepted) {
        QString agentName = dialog.getAgentName();
        QStringList directories = dialog.getDirectories();
        QString configPath = dialog.getConfigPath();

        if (m_runningAgents.contains(agentName)) {
            QMessageBox::warning(this, "Ошибка",
                QString("Агент с именем '%1' уже запущен").arg(agentName));
            return;
        }

        QStringList args;
        if (!configPath.isEmpty()) {
            args << "-c" << configPath;
        }
        args << "-n" << agentName;
        args << directories;

        QProcess *agentProcess = new QProcess(this);
        QString programPath = QCoreApplication::applicationDirPath() + "/dlp-agent";
        agentProcess->setProgram(programPath);
        agentProcess->setArguments(args);

        m_runningAgents[agentName] = agentProcess;
        m_agentDirectories[agentName] = directories;

        // Подключение сигналов процесса
        connect(agentProcess, &QProcess::started, [this, agentName, directories]() {
            m_statusLabel->setText(QString("Агент '%1' запущен").arg(agentName));
            QMessageBox::information(this, "Успех",
                QString("Агент '%1' успешно запущен.\nМониторинг директорий: %2")
                .arg(agentName)
                .arg(directories.join(", ")));
        });

        connect(agentProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [this, agentName](int exitCode, QProcess::ExitStatus exitStatus) {
                if (exitStatus == QProcess::CrashExit || exitCode != 0) {
                    m_statusLabel->setText(QString("Агент '%1' завершился с ошибкой").arg(agentName));
                }

                if (m_runningAgents.contains(agentName)) {
                    delete m_runningAgents[agentName];
                    m_runningAgents.remove(agentName);
                    m_agentDirectories.remove(agentName);
                }
            });

        agentProcess->start();
    }
}

void MainWindow::onAgentDoubleClicked(const QModelIndex &index) {
    if (!index.isValid()) return;
    int row = index.row();
    QString agentName = m_agentsModel->item(row, 0)->text();

    if (m_runningAgents.contains(agentName)) {
        QProcess* agentProcess = m_runningAgents[agentName];

        int serverAgentId = 0;
        for (const Agent& agent : m_agents) {
            if (agent.hostname == agentName) {
                serverAgentId = agent.id;
                break;
            }
        }

        QStringList dirs = m_agentDirectories.value(agentName);
        AgentInfoDialog dialog(agentName, dirs,
                              agentProcess, serverAgentId, this);

        connect(&dialog, &AgentInfoDialog::deleteAgentRequested,
                this, &MainWindow::onDeleteAgent);

        dialog.exec();

        } else {
            QMessageBox::information(this, "Информация",
                QString("Агент '%1' не запущен локально. Управление возможно только через сервер.").arg(agentName));
    }
}

// ==================== Слоты ответов API ====================

void MainWindow::onPoliciesFetched(const QJsonArray &policies) {
    m_policies.clear();
    m_policiesModel->removeRows(0, m_policiesModel->rowCount());

    for (const QJsonValue &value : policies) {
        DlpPolicy policy = DlpPolicy::fromJson(value.toObject());
        m_policies.append(policy);

        QList<QStandardItem*> items;
        items.append(new QStandardItem(policy.name));
        items.append(new QStandardItem(policy.pattern));
        items.append(new QStandardItem(policy.severity));
        items.append(new QStandardItem(policy.isActive ? "Да" : "Нет"));

        m_policiesModel->appendRow(items);
    }

    updateStatusBar();
    m_statusLabel->setText(QString("Политики загружены: %1").arg(m_policies.size()));
}

void MainWindow::onIncidentsFetched(const QJsonArray &incidents) {
    m_incidents.clear();
    m_incidentsModel->removeRows(0, m_incidentsModel->rowCount());

    for (const QJsonValue &value : incidents) {
        Incident incident = Incident::fromJson(value.toObject());
        m_incidents.append(incident);

        QList<QStandardItem*> items;

        QStandardItem *fileItem = new QStandardItem(incident.fileName);
        fileItem->setEditable(false);
        items.append(fileItem);

        QStandardItem *filePath = new QStandardItem(incident.filePath);
        filePath->setEditable(false);
        items.append(filePath);

        QStandardItem *severityItem = new QStandardItem(incident.severity);
        severityItem->setEditable(false);
        if (incident.severity == "critical") severityItem->setBackground(Qt::red);
        else if (incident.severity == "high") severityItem->setBackground(QColor(255, 165, 0));
        else if (incident.severity == "medium") severityItem->setBackground(Qt::yellow);
        else if (incident.severity == "low") severityItem->setBackground(Qt::green);
        items.append(severityItem);

        QStandardItem *statusItem = new QStandardItem(incident.status);
        statusItem->setEditable(false);
        items.append(statusItem);

        QStandardItem *policyItem = new QStandardItem(incident.policyName);
        policyItem->setEditable(false);
        items.append(policyItem);

        QStandardItem *agentItem = new QStandardItem(incident.agentHostname);
        agentItem->setEditable(false);
        items.append(agentItem);

        QStandardItem *timeItem = new QStandardItem(incident.createdAt.toString("dd.MM.yyyy HH:mm"));
        timeItem->setEditable(false);
        items.append(timeItem);

        m_incidentsModel->appendRow(items);
    }

    updateStatusBar();
    onFilterChanged();
}

void MainWindow::onEventsFetched(const QJsonArray &events) {
    m_events.clear();
    m_eventsModel->removeRows(0, m_eventsModel->rowCount());

    for (const QJsonValue &value : events) {
        Event event = Event::fromJson(value.toObject());
        m_events.append(event);

        QList<QStandardItem*> items;
        items.append(new QStandardItem(event.agentName.isEmpty() ? event.agentId : event.agentName));
        items.append(new QStandardItem(event.fileName));
        items.append(new QStandardItem(event.eventType));
        items.append(new QStandardItem(event.isViolation ? "Да" : "Нет"));
        items.append(new QStandardItem(event.detectedAt.toString("dd.MM.yyyy HH:mm")));

        m_eventsModel->appendRow(items);
    }
    m_statusLabel->setText(QString("События загружены: %1").arg(m_events.size()));
}

void MainWindow::onAgentsFetched(const QJsonArray &agents) {
    m_agents.clear();
    m_agentsModel->removeRows(0, m_agentsModel->rowCount());

    int onlineCount;
    for (const QJsonValue &value : agents) {
        Agent agent = Agent::fromJson(value.toObject());
        m_agents.append(agent);
        if (agent.isOnline) onlineCount++;

        QList<QStandardItem*> items;
        items.append(new QStandardItem(agent.hostname));
        items.append(new QStandardItem(agent.ipAddress));
        items.append(new QStandardItem(agent.osInfo));

        QStandardItem *lastSeenItem = new QStandardItem(
            agent.lastSeen.toString("dd.MM.yyyy HH:mm:ss"));

        if (agent.isOnline) {
            lastSeenItem->setForeground(Qt::darkGreen);
        } else {
            lastSeenItem->setForeground(Qt::gray);
        }

        items.append(lastSeenItem);
        m_agentsModel->appendRow(items);
    }

    m_statusLabel->setText(QString("Агенты загружены: %1 онлайн").arg(
        count_if(m_agents.begin(), m_agents.end(),
                      [](const Agent &a) { return a.isOnline; })));
}

void MainWindow::onStatisticsFetched(const QJsonObject &stats) {
    StatsData data;

    data.totalIncidents = stats["total_incidents"].toInt();
    data.newIncidents = stats["new_incidents"].toInt();
    data.investigating = stats["investigating"].toInt();
    data.resolved = stats["resolved"].toInt();
    data.falsePositive = stats["false_positive"].toInt();

    QJsonArray severityStats = stats["severity_stats"].toArray();
    for (const QJsonValue &severity : severityStats) {
        QJsonObject sevObj = severity.toObject();
        QString sev = sevObj["severity"].toString();
        int count = sevObj["count"].toInt();

        if (sev == "critical") data.severity.critical = count;
        else if (sev == "high") data.severity.high = count;
        else if (sev == "medium") data.severity.medium = count;
        else if (sev == "low") data.severity.low = count;
        else if (sev == "info") data.severity.info = count;
    }

    QJsonArray policyStats = stats["policy_stats"].toArray();
    for (const QJsonValue &policy : policyStats) {
        QJsonObject policyObj = policy.toObject();
        QString policyName = policyObj["policy_name"].toString();
        int count = policyObj["count"].toInt();
        data.incidentsByPolicy[policyName] = count;
    }

    QJsonArray agentStats = stats["agent_stats"].toArray();
    for (const QJsonValue &agent : agentStats) {
        QJsonObject agentObj = agent.toObject();
        QString hostname = agentObj["hostname"].toString();
        int count = agentObj["count"].toInt();
        data.incidentsByAgent[hostname] = count;
    }

    StatisticsTabWidget *statsWidget = m_statisticsTab->findChild<StatisticsTabWidget*>();
    if (statsWidget) {
        statsWidget->updateStatistics(data);
    }

    updateStatusBar();
    m_statusLabel->setText("Статистика обновлена");
}

void MainWindow::onErrorOccurred(const QString &error) {
    m_statusLabel->setText("Ошибка: " + error);
    QMessageBox::warning(this, "Ошибка", error);
}

void MainWindow::onPolicyCreated(const QJsonObject &policy) {
    m_apiClient->fetchPolicies();
    m_statusLabel->setText("Политика успешно создана");
}

void MainWindow::onPolicyUpdated(int id) {
    m_apiClient->fetchPolicies();
    m_statusLabel->setText(QString("Политика #%1 успешно обновлена").arg(id));
}

void MainWindow::onPolicyDeleted(int id) {
    m_apiClient->fetchPolicies();
    m_statusLabel->setText(QString("Политика #%1 успешно удалена").arg(id));
}

void MainWindow::onIncidentStatusUpdated(int id) {
    m_apiClient->fetchIncidents();
    m_statusLabel->setText(QString("Статус инцидента #%1 изменен").arg(id));
}


// ==================== Слоты обновления интерфейса ====================

void MainWindow::updateStatusBar() {
    int incidentsCount = m_incidents.size();
    int policiesCount = m_policies.size();
    int agentsCount = m_agents.size();

    QString statsText = QString("Инциденты: %1 | Политики: %2 | Агенты: %3")
                       .arg(incidentsCount).arg(policiesCount).arg(agentsCount);

    m_statsLabel->setText(statsText);
}

void MainWindow::onFilterChanged() {
    QString severityFilter = m_severityFilter->currentText();
    QString statusFilter = m_statusFilter->currentText();

    m_incidentsProxyModel->setFilterKeyColumn(-1);

    for (int i = 0; i < m_incidentsProxyModel->rowCount(); ++i) {
        m_incidentsTable->setRowHidden(i, false);
    }

    if (severityFilter != "Все" && statusFilter != "Все") {
        m_incidentsProxyModel->setFilterFixedString("");

        for (int i = 0; i < m_incidentsProxyModel->rowCount(); ++i) {
            QString severity = m_incidentsProxyModel->data(m_incidentsProxyModel->index(i, 3)).toString();
            QString status = m_incidentsProxyModel->data(m_incidentsProxyModel->index(i, 4)).toString();

            bool match = (severity == severityFilter && status == statusFilter);
            m_incidentsTable->setRowHidden(i, !match);
        }
    }
    else if (severityFilter != "Все") {
        m_incidentsProxyModel->setFilterFixedString(severityFilter);
    }
    else if (statusFilter != "Все") {
        m_incidentsProxyModel->setFilterFixedString(statusFilter);
    }
    else {
        m_incidentsProxyModel->setFilterFixedString("");
    }
}

void MainWindow::onDeleteAgent(int agentId) {
    if (agentId <= 0) {
        m_statusLabel->setText("ID агента не найден");
        return;
    }

    QNetworkAccessManager* manager = new QNetworkAccessManager(this);
    QUrl url(m_apiClient->getBaseUrl() + QString("/api/v1/agents/%1").arg(agentId));
    QNetworkRequest request(url);

    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply* reply = manager->deleteResource(request);

    connect(reply, &QNetworkReply::finished, [this, reply, agentId, manager]() {
        reply->deleteLater();
        manager->deleteLater();

        if (reply->error() == QNetworkReply::NoError) {
            m_statusLabel->setText(QString("Агент #%1 удален").arg(agentId));
            m_apiClient->fetchAgents();

            QString agentNameToRemove;
            for (auto it = m_runningAgents.begin(); it != m_runningAgents.end(); ++it) {}

            if (!agentNameToRemove.isEmpty()) {
                m_runningAgents.remove(agentNameToRemove);
            }
        } else {
            QString error = QString("Ошибка удаления агента #%1: %2")
                              .arg(agentId)
                              .arg(reply->errorString());
            m_statusLabel->setText(error);
            QMessageBox::warning(this, "Ошибка", error);
        }
    });
}
