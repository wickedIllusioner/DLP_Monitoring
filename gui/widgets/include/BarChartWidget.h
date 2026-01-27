#ifndef BARCHARTWIDGET_H
#define BARCHARTWIDGET_H

#include "ChartWidget.h"
#include <QtCharts/QBarSeries>

class BarChartWidget : public ChartWidget {
    Q_OBJECT

public:
    explicit BarChartWidget(QWidget* parent = nullptr);

    void setData(const QMap<QString, int>& data);
    void setCategories(const QStringList& categories);
    void clearData();

    QMap<QString, int> data() const;
    QStringList categories() const;

protected:
    void updateChart() override;

private:
    QBarSeries* m_series;
    QMap<QString, int> m_data;
    QStringList m_categories;
};

#endif //BARCHARTWIDGET_H
