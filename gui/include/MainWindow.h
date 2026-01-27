#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QComboBox>
#include <QLabel>
#include <QMainWindow>
#include <QTabWidget>
#include <QTableView>
#include <QPushButton>
#include <QStatusBar>
#include <QTimer>
#include <QDateEdit>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include "ApiClient.h"
#include "DataModels.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() {}

private slots:
    // Слоты для кнопок
    void onAddPolicy();
    void onEditPolicy();
    void onDeletePolicy();
    void onRefreshData();
    void onChangeIncidentStatus();

    // Слоты ответов API
    void onPoliciesFetched(const QJsonArray &policies);
    void onIncidentsFetched(const QJsonArray &incidents);
    void onEventsFetched(const QJsonArray &events);
    void onAgentsFetched(const QJsonArray &agents);
    void onStatisticsFetched(const QJsonObject &stats);
    void onErrorOccurred(const QString &error);
    void onPolicyCreated(const QJsonObject& policy);
    void onPolicyUpdated(int id);
    void onPolicyDeleted(int id);
    void onIncidentStatusUpdated(int id);
    void onFilterChanged();

    // Слоты обновления интерфейса
    void updateStatusBar();

private:
    void setupUI();
    void setupConnections();
    void setupMenuBar();
    void setupToolBar();
    void setupTabs();
    void setupStatusBar();

    // Компоненты интерфейса
    QTabWidget *m_tabWidget;

    // Виджеты для вкладок
    QWidget *m_incidentsTab;
    QWidget *m_policiesTab;
    QWidget *m_eventsTab;
    QWidget *m_agentsTab;
    QWidget *m_statisticsTab;

    // Таблицы
    QTableView *m_incidentsTable;
    QTableView *m_policiesTable;
    QTableView *m_eventsTable;
    QTableView *m_agentsTable;

    // Кнопки и элементы управления
    QPushButton *m_btnAddPolicy;
    QPushButton *m_btnEditPolicy;
    QPushButton *m_btnDeletePolicy;
    QPushButton *m_btnRefresh;
    QPushButton *m_btnChangeStatus;

    // Фильтры
    QComboBox *m_severityFilter;
    QComboBox *m_statusFilter;

    // Статус бар
    QStatusBar *m_statusBar;
    QLabel *m_statusLabel;
    QLabel *m_statsLabel;

    // API клиент
    ApiClient *m_apiClient;

    // Данные
    QList<DlpPolicy> m_policies;
    QList<Incident> m_incidents;
    QList<Event> m_events;
    QList<Agent> m_agents;

    // Модели данных
    QStandardItemModel *m_incidentsModel;
    QStandardItemModel *m_policiesModel;
    QStandardItemModel *m_eventsModel;
    QStandardItemModel *m_agentsModel;

    // Таймер автообновления
    QTimer *m_autoRefreshTimer;

    QSortFilterProxyModel* m_incidentsProxyModel;
};

#endif //MAINWINDOW_H