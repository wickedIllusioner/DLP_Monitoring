#include "../include/StatisticsTabWidget.h"
#include <QScrollArea>

StatisticsTabWidget::StatisticsTabWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
}

void StatisticsTabWidget::setupUI() {
    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setMinimumHeight(600);

    QWidget *scrollWidget = new QWidget();
    m_mainLayout = new QVBoxLayout(scrollWidget);
    m_mainLayout->setSpacing(15);

    // 1. Сводка
    m_summaryWidget = new StatsSummaryWidget();
    m_mainLayout->addWidget(m_summaryWidget);

    // 2. Круговые диаграммы
    m_topLayout = new QHBoxLayout();
    m_statusPieChart = new PieChartWidget();
    m_statusPieChart->setTitle("По статусам");
    m_statusPieChart->setMinimumHeight(300);
    m_topLayout->addWidget(m_statusPieChart);

    m_severityPieChart = new PieChartWidget();
    m_severityPieChart->setTitle("По серьезности");
    m_severityPieChart->setMinimumHeight(300);
    m_topLayout->addWidget(m_severityPieChart);
    m_mainLayout->addLayout(m_topLayout);

    // 3. Столбчатые диаграммы
    m_middleLayout = new QHBoxLayout();
    m_policiesBarChart = new BarChartWidget();
    m_policiesBarChart->setTitle("По политикам");
    m_policiesBarChart->setMinimumHeight(350);
    m_middleLayout->addWidget(m_policiesBarChart);

    m_agentsBarChart = new BarChartWidget();
    m_agentsBarChart->setTitle("По агентам");
    m_policiesBarChart->setMinimumHeight(350);
    m_middleLayout->addWidget(m_agentsBarChart);
    m_mainLayout->addLayout(m_middleLayout);

    m_mainLayout->addStretch();
    scrollArea->setWidget(scrollWidget);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(scrollArea);
    setLayout(mainLayout);
}

void StatisticsTabWidget::updateStatistics(const StatsData &data) {
    // Сводка
    QMap<QString, QString> summaryData;
    summaryData["Всего инцидентов"] = QString::number(data.totalIncidents);
    summaryData["Новые"] = QString::number(data.newIncidents);
    summaryData["В расследовании"] = QString::number(data.investigating);
    summaryData["Решено"] = QString::number(data.resolved);
    summaryData["Ложные"] = QString::number(data.falsePositive);
    summaryData["Онлайн агентов"] = QString("%1/%2").arg(data.onlineAgents).arg(data.totalAgents);
    summaryData["Активных политик"] = QString::number(data.activePolicies);
    m_summaryWidget->setData(summaryData);

    // Статусы
    QMap<QString, int> statusData;
    statusData["Новые"] = data.newIncidents;
    statusData["В расследовании"] = data.investigating;
    statusData["Решено"] = data.resolved;
    statusData["Ложные"] = data.falsePositive;
    m_statusPieChart->setData(statusData);

    // Серьезность
    QMap<QString, int> severityData;
    severityData["Критический"] = data.severity.critical;
    severityData["Высокий"] = data.severity.high;
    severityData["Средний"] = data.severity.medium;
    severityData["Низкий"] = data.severity.low;
    severityData["Инфо"] = data.severity.info;
    m_severityPieChart->setData(severityData);

    // По политикам
    m_policiesBarChart->setData(data.incidentsByPolicy);

    // По агентам
    QMap<QString, int> topAgents;
    auto agentsList = data.incidentsByAgent.keys();
    std::sort(agentsList.begin(), agentsList.end(),
              [&](const QString &a, const QString &b) {
                  return data.incidentsByAgent[a] > data.incidentsByAgent[b];
              });

    for (int i = 0; i < qMin(10, agentsList.size()); ++i) {
        topAgents[agentsList[i]] = data.incidentsByAgent[agentsList[i]];
    }
    m_agentsBarChart->setData(topAgents);
}

void StatisticsTabWidget::clearStatistics() {
    m_summaryWidget->clearData();
    m_statusPieChart->clearData();
    m_severityPieChart->clearData();
    m_policiesBarChart->clearData();
    m_agentsBarChart->clearData();
}