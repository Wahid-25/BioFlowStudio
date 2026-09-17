#include "ProjectSerializer.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <stdexcept>

void ProjectSerializer::save(
    const QString& filePath,
    const ProjectSession& session
)
{
    QJsonObject root;
    root["format"] = "BioFlow Studio Project";
    root["version"] = 1;
    root["projectName"] = session.projectName;
    root["savedAt"] = session.savedAt;

    QJsonArray fastaFiles;

    for (const QString& path : session.fastaFilePaths)
    {
        fastaFiles.append(path);
    }

    root["fastaFiles"] = fastaFiles;
    root["expressionFile"] = session.expressionFilePath;

    QJsonObject settings;
    settings["matrixMethodIndex"] = session.matrixMethodIndex;
    settings["treeMethodIndex"] = session.treeMethodIndex;
    settings["normalizationMethodIndex"] = session.normalizationMethodIndex;
    settings["adjustedPValueThreshold"] = session.adjustedPValueThreshold;
    settings["foldChangeThreshold"] = session.foldChangeThreshold;
    settings["enrichmentPValueThreshold"] = session.enrichmentPValueThreshold;
    settings["workflowTemplateIndex"] = session.workflowTemplateIndex;

    QJsonArray groups;

    for (int group : session.sampleGroups)
    {
        groups.append(group);
    }

    settings["sampleGroups"] = groups;
    root["settings"] = settings;

    QJsonArray history;

    for (const AnalysisHistoryEntry& entry : session.history)
    {
        QJsonObject item;
        item["timestamp"] = entry.timestamp;
        item["workspace"] = entry.workspace;
        item["action"] = entry.action;
        item["status"] = entry.status;
        item["details"] = entry.details;
        history.append(item);
    }

    root["history"] = history;

    QFile file(filePath);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        throw std::runtime_error("Could not open the project file for writing.");
    }

    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
}

ProjectSession ProjectSerializer::load(const QString& filePath)
{
    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly))
    {
        throw std::runtime_error("Could not open the selected project file.");
    }

    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(
        file.readAll(),
        &parseError
    );

    if (parseError.error != QJsonParseError::NoError
        || !document.isObject())
    {
        throw std::runtime_error("The selected file is not valid BioFlow JSON.");
    }

    QJsonObject root = document.object();

    if (root.value("format").toString() != "BioFlow Studio Project")
    {
        throw std::runtime_error("The selected file is not a BioFlow project.");
    }

    ProjectSession session;
    session.projectName = root.value("projectName").toString(
        "Untitled BioFlow Project"
    );
    session.savedAt = root.value("savedAt").toString();

    for (const QJsonValue& value : root.value("fastaFiles").toArray())
    {
        session.fastaFilePaths.append(value.toString());
    }

    session.expressionFilePath = root.value("expressionFile").toString();

    QJsonObject settings = root.value("settings").toObject();
    session.matrixMethodIndex = settings.value("matrixMethodIndex").toInt(0);
    session.treeMethodIndex = settings.value("treeMethodIndex").toInt(0);
    session.normalizationMethodIndex = settings.value(
        "normalizationMethodIndex"
    ).toInt(1);
    session.adjustedPValueThreshold = settings.value(
        "adjustedPValueThreshold"
    ).toDouble(0.05);
    session.foldChangeThreshold = settings.value(
        "foldChangeThreshold"
    ).toDouble(1.0);
    session.enrichmentPValueThreshold = settings.value(
        "enrichmentPValueThreshold"
    ).toDouble(1.0);
    session.workflowTemplateIndex = settings.value(
        "workflowTemplateIndex"
    ).toInt(0);

    for (const QJsonValue& value : settings.value("sampleGroups").toArray())
    {
        session.sampleGroups.push_back(value.toInt(0));
    }

    for (const QJsonValue& value : root.value("history").toArray())
    {
        QJsonObject item = value.toObject();
        session.history.push_back({
            item.value("timestamp").toString(),
            item.value("workspace").toString(),
            item.value("action").toString(),
            item.value("status").toString(),
            item.value("details").toString()
        });
    }

    return session;
}
