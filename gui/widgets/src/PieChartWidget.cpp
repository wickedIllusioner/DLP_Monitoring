#include "../include/PieChartWidget.h"

PieChartWidget::PieChartWidget(QWidget *parent)
    : ChartWidget(parent)
{
    m_series = new QPieSeries();
    m_chart->addSeries(m_series);
    setTitle("Распределение");
}

void PieChartWidget::setData(const QMap<QString, int> &data) {
    m_data = data;
    updateChart();
}

void PieChartWidget::clearData() {
    m_data.clear();
    m_series->clear();
}

QMap<QString, int> PieChartWidget::data() const {
    return m_data;
}

void PieChartWidget::updateChart() {
    m_series->clear();
    for (auto it = m_data.constBegin(); it != m_data.constEnd(); ++it) {
        QPieSlice* slice = m_series->append(it.key(), it.value());
        slice->setLabelVisible(true);
        slice->setLabel(QString("%1 (%2)")
            .arg(it.key())
            .arg(it.value()));
    }
}





