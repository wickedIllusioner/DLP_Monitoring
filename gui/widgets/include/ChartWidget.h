#ifndef CHARTWIDGET_H
#define CHARTWIDGET_H

#include <QWidget>
#include <QtCharts/QChartView>

class ChartWidget : public QWidget {
    Q_OBJECT

public:
    explicit ChartWidget(QWidget *parent = nullptr);
    virtual ~ChartWidget() = default;

    void setTitle(const QString &title);
    QString title() const;

    void setAnimationEnabled(bool enabled);
    bool isAnimationEnabled() const;

    void setLegendVisible(bool visible);
    bool isLegendVisible() const;

protected:
    QChartView *m_chartView;
    QChart *m_chart;

    virtual void setupChart();
    virtual void updateChart() = 0;

private:
    QString m_title;
    bool m_animationEnabled;
    bool m_legendVisible;
};


#endif //CHARTWIDGET_H
