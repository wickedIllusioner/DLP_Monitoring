#include "../include/ChartWidget.h"
#include <QtCharts/QChart>
#include <QVBoxLayout>

ChartWidget::ChartWidget(QWidget *parent)
    : QWidget(parent)
    , m_animationEnabled(true)
    , m_legendVisible(true)
{
    m_chartView = new QChartView(this);
    m_chart = new QChart();

    setupChart();

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(m_chartView);
    layout->setContentsMargins(0, 0, 0, 0);
    setLayout(layout);
}

void ChartWidget::setupChart()
{
    m_chart->setAnimationOptions(m_animationEnabled ?
        QChart::SeriesAnimations : QChart::NoAnimation);

    m_chart->legend()->setVisible(m_legendVisible);
    m_chartView->setChart(m_chart);
    m_chartView->setRenderHint(QPainter::Antialiasing);
}

void ChartWidget::setTitle(const QString &title)
{
    m_title = title;
    m_chart->setTitle(title);
}

QString ChartWidget::title() const
{
    return m_title;
}

void ChartWidget::setAnimationEnabled(bool enabled)
{
    if (m_animationEnabled != enabled) {
        m_animationEnabled = enabled;
        m_chart->setAnimationOptions(enabled ?
            QChart::SeriesAnimations : QChart::NoAnimation);
    }
}

bool ChartWidget::isAnimationEnabled() const
{
    return m_animationEnabled;
}

void ChartWidget::setLegendVisible(bool visible)
{
    if (m_legendVisible != visible) {
        m_legendVisible = visible;
        m_chart->legend()->setVisible(visible);
    }
}

bool ChartWidget::isLegendVisible() const
{
    return m_legendVisible;
}