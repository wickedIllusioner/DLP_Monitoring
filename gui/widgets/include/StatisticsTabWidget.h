#ifndef STATSTABWIDGET_H
#define STATSTABWIDGET_H

#include "StatsData.h"
#include "PieChartWidget.h"
#include "BarChartWidget.h"
#include "StatsSummaryWidget.h"
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>


class StatisticsTabWidget : public QWidget {
    Q_OBJECT

public:
    explicit StatisticsTabWidget(QWidget *parent = nullptr);

    void updateStatistics(const StatsData &data);
    void clearStatistics();

private:
    void setupUI();

    StatsSummaryWidget *m_summaryWidget;
    PieChartWidget *m_statusPieChart;
    PieChartWidget *m_severityPieChart;
    BarChartWidget *m_policiesBarChart;
    BarChartWidget *m_agentsBarChart;

    QVBoxLayout *m_mainLayout;
    QHBoxLayout *m_topLayout;
    QHBoxLayout *m_middleLayout;
    QHBoxLayout *m_bottomLayout;
};


#endif //STATSTABWIDGET_H
