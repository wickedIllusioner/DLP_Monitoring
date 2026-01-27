#ifndef STATSDATA_H
#define STATSDATA_H

#include <QMap>
#include <QString>
#include <QList>
#include <QPair>
#include <QDateTime>

struct SeverityStats {
    int critical = 0;
    int high = 0;
    int medium = 0;
    int low = 0;
    int info = 0;
};

struct StatsData {
    int totalIncidents = 0;
    int newIncidents = 0;
    int investigating = 0;
    int resolved = 0;
    int falsePositive = 0;
    SeverityStats severity;

    int onlineAgents = 0;
    int totalAgents = 0;
    int activePolicies = 0;

    QMap<QString, int> incidentsByPolicy;
    QMap<QString, int> incidentsByAgent;
    QList<QPair<QDateTime, int>> timeline;
};

#endif //STATSDATA_H
