#include "../include/BarChartWidget.h"
#include <QtCharts/QBarSet>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QValueAxis>

BarChartWidget::BarChartWidget(QWidget *parent)
    : ChartWidget(parent)
{
    m_series = new QBarSeries();
    m_chart->addSeries(m_series);
    setTitle("Столбчатая диаграмм");
}

void BarChartWidget::setData(const QMap<QString, int> &data) {
    m_data = data;
    updateChart();
}


void BarChartWidget::setCategories(const QStringList &categories) {
    m_categories = categories;
    updateChart();
}

void BarChartWidget::clearData() {
    m_data.clear();
    m_series->clear();
}

QMap<QString, int> BarChartWidget::data() const {
    return m_data;
}

QStringList BarChartWidget::categories() const {
    return m_categories;
}

void BarChartWidget::updateChart() {
    m_chart->removeAllSeries();

    for (QAbstractAxis* axis : m_chart->axes()) {
        m_chart->removeAxis(axis);
        delete axis;
    }

    if (m_data.isEmpty()) {
        return;
    }

    QBarSeries *newSeries = new QBarSeries();
    QBarSet *barSet = new QBarSet("Количество");

    QStringList labels;
    int maxValue = 0;
    for (auto it = m_data.constBegin(); it != m_data.constEnd(); ++it) {
        *barSet << it.value();
        labels << it.key();
        if (it.value() > maxValue) { maxValue = it.value(); }
    }

    newSeries->append(barSet);
    m_chart->addSeries(newSeries);

    QBarCategoryAxis *axisX = new QBarCategoryAxis();
    axisX->append(labels);

    QValueAxis *axisY = new QValueAxis();
    axisY->setLabelFormat("%d");
    axisY->setTitleText("Количество");
    axisY->setRange(0, maxValue+1);
    axisY->setTickType(QValueAxis::TicksFixed);

    m_chart->addAxis(axisX, Qt::AlignBottom);
    m_chart->addAxis(axisY, Qt::AlignLeft);

    newSeries->attachAxis(axisX);
    newSeries->attachAxis(axisY);

    m_series = newSeries;
}

