#include "IncidentsTable.h"
#include <QHeaderView>
#include <QFileDialog>
#include <QTextStream>
#include <QMessageBox>
#include <QMouseEvent>
#include <QDateTime>
#include <QColor>
#include <algorithm>

IncidentsTable::IncidentsTable(QWidget *parent)
    : QTableView(parent)
    , m_model(new QStandardItemModel(this))
    , m_proxyModel(new QSortFilterProxyModel(this))
    , m_contextMenu(new QMenu(this))
{
    setupUI();
    setupContextMenu();

    // Настройка модели
    m_proxyModel->setSourceModel(m_model);
    m_proxyModel->setSortRole(Qt::UserRole + 1);

    setModel(m_proxyModel);
    setSortingEnabled(true);

    // Соединение сигналов
    connect(selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &IncidentsTable::onSelectionChanged);
}

void IncidentsTable::setupUI()
{
    // Настройка таблицы
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setAlternatingRowColors(true);

    // Настройка заголовков
    QStringList headers;
    headers << "ID" << "Файл" << "Путь" << "Серьезность" << "Политика"
            << "Агент" << "Статус" << "Содержимое" << "Время создания";

    m_model->setHorizontalHeaderLabels(headers);

    // Настройка колонок
    horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents); // ID
    horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents); // Файл
    horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);         // Путь
    horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents); // Серьезность
    horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents); // Политика
    horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents); // Агент
    horizontalHeader()->setSectionResizeMode(6, QHeaderView::ResizeToContents); // Статус
    horizontalHeader()->setSectionResizeMode(7, QHeaderView::Stretch);         // Содержимое
    horizontalHeader()->setSectionResizeMode(8, QHeaderView::ResizeToContents); // Время

    verticalHeader()->setVisible(false);
}

void IncidentsTable::setupContextMenu()
{
    m_actionNew = m_contextMenu->addAction("Отметить как 'Новый'");
    m_actionInvestigating = m_contextMenu->addAction("Отметить как 'В расследовании'");
    m_actionResolved = m_contextMenu->addAction("Отметить как 'Решен'");
    m_actionFalsePositive = m_contextMenu->addAction("Отметить как 'Ложное срабатывание'");
    m_contextMenu->addSeparator();
    m_actionExport = m_contextMenu->addAction("Экспорт в CSV...");

    connect(m_actionNew, &QAction::triggered, this, &IncidentsTable::onMarkAsNew);
    connect(m_actionInvestigating, &QAction::triggered, this, &IncidentsTable::onMarkAsInvestigating);
    connect(m_actionResolved, &QAction::triggered, this, &IncidentsTable::onMarkAsResolved);
    connect(m_actionFalsePositive, &QAction::triggered, this, &IncidentsTable::onMarkAsFalsePositive);
    connect(m_actionExport, &QAction::triggered, this, &IncidentsTable::onExportToCSV);
}

void IncidentsTable::setIncidents(const QList<Incident> &incidents)
{
    clear();
    m_incidents = incidents;

    m_model->setRowCount(incidents.size());

    for (int i = 0; i < incidents.size(); ++i) {
        const Incident &incident = incidents[i];

        // ID
        QStandardItem *idItem = new QStandardItem(QString::number(incident.id));
        idItem->setData(incident.id, Qt::UserRole + 1);
        m_model->setItem(i, 0, idItem);

        // Имя файла
        QStandardItem *fileItem = new QStandardItem(incident.fileName);
        fileItem->setToolTip(incident.filePath);
        m_model->setItem(i, 1, fileItem);

        // Путь к файлу
        QStandardItem *pathItem = new QStandardItem(incident.filePath);
        pathItem->setToolTip(incident.filePath);
        m_model->setItem(i, 2, pathItem);

        // Серьезность с цветом
        QStandardItem *severityItem = new QStandardItem(incident.severity);
        severityItem->setBackground(getSeverityColor(incident.severity));
        severityItem->setForeground(Qt::black);
        severityItem->setTextAlignment(Qt::AlignCenter);
        m_model->setItem(i, 3, severityItem);

        // Название политики
        m_model->setItem(i, 4, new QStandardItem(incident.policyName));

        // Агент
        m_model->setItem(i, 5, new QStandardItem(incident.agentHostname));

        // Статус с цветом
        QStandardItem *statusItem = new QStandardItem(incident.status);
        if (incident.status == "new") statusItem->setBackground(Qt::yellow);
        else if (incident.status == "investigating") statusItem->setBackground(Qt::cyan);
        else if (incident.status == "resolved") statusItem->setBackground(Qt::green);
        else if (incident.status == "false_positive") statusItem->setBackground(Qt::gray);
        statusItem->setTextAlignment(Qt::AlignCenter);
        m_model->setItem(i, 6, statusItem);

        // Обнаруженное содержимое
        QStandardItem *contentItem = new QStandardItem(incident.matchedContent);
        contentItem->setToolTip(incident.matchedContent);
        m_model->setItem(i, 7, contentItem);

        // Время создания
        QStandardItem *timeItem = new QStandardItem(incident.createdAt.toString("dd.MM.yyyy HH:mm:ss"));
        timeItem->setData(incident.createdAt, Qt::UserRole + 1);
        m_model->setItem(i, 8, timeItem);
    }

    sortByColumn(8, Qt::DescendingOrder);
}

void IncidentsTable::clear()
{
    m_model->removeRows(0, m_model->rowCount());
    m_incidents.clear();
}

Incident IncidentsTable::selectedIncident() const
{
    QModelIndexList selected = selectionModel()->selectedRows();
    if (selected.isEmpty()) return Incident();

    int sourceRow = m_proxyModel->mapToSource(selected.first()).row();
    if (sourceRow >= 0 && sourceRow < m_incidents.size()) {
        return m_incidents[sourceRow];
    }

    return Incident();
}

QList<Incident> IncidentsTable::selectedIncidents() const
{
    QList<Incident> selected;
    QModelIndexList selectedIndexes = selectionModel()->selectedRows();

    for (const QModelIndex &index : selectedIndexes) {
        int sourceRow = m_proxyModel->mapToSource(index).row();
        if (sourceRow >= 0 && sourceRow < m_incidents.size()) {
            selected.append(m_incidents[sourceRow]);
        }
    }

    return selected;
}

QColor IncidentsTable::getSeverityColor(const QString &severity) const
{
    if (severity == "critical") return QColor(255, 200, 200); // Красный
    if (severity == "high") return QColor(255, 225, 200);     // Оранжевый
    if (severity == "medium") return QColor(255, 255, 200);   // Желтый
    if (severity == "low") return QColor(200, 255, 200);      // Зеленый
    return QColor(240, 240, 240);                            // Серый для info
}

void IncidentsTable::contextMenuEvent(QContextMenuEvent *event)
{
    if (!selectedIncidents().isEmpty()) {
        m_contextMenu->exec(event->globalPos());
    }
}

void IncidentsTable::mouseDoubleClickEvent(QMouseEvent *event)
{
    QTableView::mouseDoubleClickEvent(event);

    Incident incident = selectedIncident();
    if (incident.id > 0) {
        emit incidentDoubleClicked(incident);
    }
}

void IncidentsTable::onSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    Q_UNUSED(deselected);

    if (!selected.isEmpty()) {
        Incident incident = selectedIncident();
        if (incident.id > 0) {
            emit incidentSelected(incident);
        }
    }
}

void IncidentsTable::onMarkAsNew()
{
    QList<Incident> selected = selectedIncidents();
    for (const Incident &incident : selected) {
        emit statusChangedRequest(incident.id, "new");
    }
}

void IncidentsTable::onMarkAsInvestigating()
{
    QList<Incident> selected = selectedIncidents();
    for (const Incident &incident : selected) {
        emit statusChangedRequest(incident.id, "investigating");
    }
}

void IncidentsTable::onMarkAsResolved()
{
    QList<Incident> selected = selectedIncidents();
    for (const Incident &incident : selected) {
        emit statusChangedRequest(incident.id, "resolved");
    }
}

void IncidentsTable::onMarkAsFalsePositive()
{
    QList<Incident> selected = selectedIncidents();
    for (const Incident &incident : selected) {
        emit statusChangedRequest(incident.id, "false_positive");
    }
}

void IncidentsTable::onExportToCSV()
{
    QString fileName = QFileDialog::getSaveFileName(
        this, "Экспорт в CSV", "", "CSV файлы (*.csv)"
    );

    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Ошибка", "Не удалось открыть файл для записи");
        return;
    }

    QTextStream stream(&file);

    QStringList headers;
    for (int i = 0; i < m_model->columnCount(); ++i) {
        headers << m_model->headerData(i, Qt::Horizontal).toString();
    }
    stream << headers.join(';') << "\n";

    for (int row = 0; row < m_model->rowCount(); ++row) {
        QStringList rowData;
        for (int col = 0; col < m_model->columnCount(); ++col) {
            QStandardItem *item = m_model->item(row, col);
            QString text = item ? item->text().replace(';', ',') : "";
            rowData << "\"" + text + "\"";
        }
        stream << rowData.join(';') << "\n";
    }

    file.close();
    emit exportRequested();
}