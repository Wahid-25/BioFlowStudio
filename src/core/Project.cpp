#include "Project.h"

Project::Project()
    : name("Untitled Project"),
      description(""),
      workspaceType(WorkspaceType::NotSelected)
{
}

Project::Project(
    const std::string& name,
    const std::string& description,
    WorkspaceType workspaceType
)
    : name(name),
      description(description),
      workspaceType(workspaceType)
{
}

const std::string& Project::getName() const
{
    return name;
}

const std::string& Project::getDescription() const
{
    return description;
}

WorkspaceType Project::getWorkspaceType() const
{
    return workspaceType;
}

void Project::setName(const std::string& name)
{
    this->name = name;
}

void Project::setDescription(const std::string& description)
{
    this->description = description;
}

void Project::setWorkspaceType(WorkspaceType workspaceType)
{
    this->workspaceType = workspaceType;
}

std::string Project::getWorkspaceName() const
{
    switch (workspaceType)
    {
        case WorkspaceType::PhylogeneticAnalysis:
            return "Phylogenetic Analysis";

        case WorkspaceType::GeneExpressionAnalysis:
            return "Gene Expression Analysis";

        default:
            return "Not Selected";
    }
}