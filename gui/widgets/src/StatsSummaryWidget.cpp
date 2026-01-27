#include "../include/StatsSummaryWidget.h"
#include <QLabel>
#include <QFrame>

StatsSummaryWidget::StatsSummaryWidget(QWidget *parent)
    : QWidget(parent)
{
    m_layout = new QGridLayout(this);
    m_layout->setSpacing(10);
    setLayout(m_layout);
}

void StatsSummaryWidget::setData(const QMap<QString, QString> &data) {
    m_data = data;
    updateDisplay();
}

void StatsSummaryWidget::clearData() {
    m_data.clear();

    QLayoutItem *child;
    while ((child = m_layout->takeAt(0)) != nullptr) {
        delete child->widget();
        delete child;
    }
}

QMap<QString, QString> StatsSummaryWidget::data() const {
    return m_data;
}

void StatsSummaryWidget::updateDisplay() {
    clearData();

    int row = 0;
    int col = 0;
    const int cols = 2;

    for (auto it = m_data.constBegin(); it != m_data.constEnd(); ++it) {
        QFrame *card = new QFrame(this);
        card->setFrameStyle(QFrame::Box | QFrame::Raised);
        card->setStyleSheet("background-color: #f5f5f5; padding: 10px;");

        QVBoxLayout *cardLayout = new QVBoxLayout(card);

        QLabel *titleLabel = new QLabel(it.key(), card);
        titleLabel->setStyleSheet("font-weight: bold; color: #333;");

        QLabel *valueLabel = new QLabel(it.value(), card);
        valueLabel->setStyleSheet("font-size: 16pt; color: #0066cc;");

        cardLayout->addWidget(titleLabel);
        cardLayout->addWidget(valueLabel);
        cardLayout->addStretch();

        m_layout->addWidget(card, row, col);

        col++;
        if (col >= cols) {
            col = 0;
            row++;
        }
    }
}