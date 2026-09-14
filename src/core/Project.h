#pragma once

#include <string>

enum class WorkspaceType
{
    NotSelected,
    PhylogeneticAnalysis,
    GeneExpressionAnalysis
};

class Project
{
private:
    std::string name;
    std::string description;
    WorkspaceType workspaceType;

public:
    Project();

    Project(
        const std::string& name,
        const std::string& description,
        WorkspaceType workspaceType
    );

    const std::string& getName() const;
    const std::string& getDescription() const;
    WorkspaceType getWorkspaceType() const;

    void setName(const std::string& name);
    void setDescription(const std::string& description);
    void setWorkspaceType(WorkspaceType workspaceType);

    std::string getWorkspaceName() const;
};