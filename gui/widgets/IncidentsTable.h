#ifndef INCIDENTSTABLE_H
#define INCIDENTSTABLE_H

#include <QWidget>
#include <QTableView>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>
#include <QMenu>
#include <QAction>
#include <QContextMenuEvent>

#include "../include/DataModels.h"


class IncidentsTable : public QTableView {
    Q_OBJECT

public:
    explicit IncidentsTable(QWidget* parent = nullptr);
    void setIncidents(const QList<Incident> &incidents);
    void clear();

    Incident selectedIncident() const;
    QList<Incident> selectedIncidents() const;

    signals:
        void incidentSelected(const Incident &incident);
    void incidentDoubleClicked(const Incident &incident);
    void statusChangedRequest(int incidentId, const QString &status);
    void exportRequested();

protected:
    void contextMenuEvent(QContextMenuEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private slots:
    void onSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
    void onMarkAsNew();
    void onMarkAsInvestigating();
    void onMarkAsResolved();
    void onMarkAsFalsePositive();
    void onExportToCSV();

private:
    void setupUI();
    void setupContextMenu();
    void updateIncidentStatus(int row, const QString &status);
    QColor getSeverityColor(const QString &severity) const;

    QStandardItemModel *m_model;
    QSortFilterProxyModel *m_proxyModel;
    QMenu *m_contextMenu;

    QList<Incident> m_incidents;

    QAction *m_actionNew;
    QAction *m_actionInvestigating;
    QAction *m_actionResolved;
    QAction *m_actionFalsePositive;
    QAction *m_actionExport;
};

#endif //INCIDENTSTABLE_H
