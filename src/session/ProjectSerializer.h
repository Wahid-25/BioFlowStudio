#pragma once

#include <QString>

#include "ProjectSession.h"

class ProjectSerializer
{
public:
    static void save(
        const QString& filePath,
        const ProjectSession& session
    );

    static ProjectSession load(const QString& filePath);
};
