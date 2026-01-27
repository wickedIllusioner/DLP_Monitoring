#ifndef STATSSUMMARYWIDGET_H
#define STATSSUMMARYWIDGET_H

#include <QWidget>
#include <QGridLayout>

class StatsSummaryWidget : public QWidget {
    Q_OBJECT

public:
    explicit StatsSummaryWidget(QWidget *parent = nullptr);

    void setData(const QMap<QString, QString> &data);
    void clearData();

    QMap<QString, QString> data() const;

private:
    void updateDisplay();

    QGridLayout *m_layout;
    QMap<QString, QString> m_data;
};

#endif //STATSSUMMARYWIDGET_H
