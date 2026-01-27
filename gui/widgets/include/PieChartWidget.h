#ifndef PIECHARTWIDGET_H
#define PIECHARTWIDGET_H

#include "ChartWidget.h"
#include <QtCharts/QPieSeries>

class PieChartWidget : public ChartWidget {
    Q_OBJECT

public:
    explicit PieChartWidget(QWidget* parent = nullptr);

    void setData(const QMap<QString, int>& data);
    void clearData();

    QMap<QString, int> data() const;

protected:
    void updateChart() override;

private:
    QPieSeries* m_series;
    QMap<QString, int> m_data;
};

#endif //PIECHARTWIDGET_H
