#pragma once

#include <QString>
#include <QStringList>

#include <vector>

struct AnalysisHistoryEntry
{
    QString timestamp;
    QString workspace;
    QString action;
    QString status;
    QString details;
};

class ProjectSession
{
public:
    QString projectName = "Untitled BioFlow Project";
    QString savedAt;
    QStringList fastaFilePaths;
    QString expressionFilePath;

    int matrixMethodIndex = 0;
    int treeMethodIndex = 0;
    int normalizationMethodIndex = 1;
    double adjustedPValueThreshold = 0.05;
    double foldChangeThreshold = 1.0;
    double enrichmentPValueThreshold = 1.0;
    int workflowTemplateIndex = 0;

    std::vector<int> sampleGroups;
    std::vector<AnalysisHistoryEntry> history;

    void addHistory(
        const QString& workspace,
        const QString& action,
        const QString& status,
        const QString& details
    );
};
