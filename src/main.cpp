#include <iostream>

#include "core/Project.h"

int main()
{
    Project project(
        "BioFlow Demonstration",
        "Testing the first core project class",
        WorkspaceType::PhylogeneticAnalysis
    );

    std::cout << "BioFlow Studio started successfully!\n";
    std::cout << "Project name: " << project.getName() << '\n';
    std::cout << "Description: " << project.getDescription() << '\n';
    std::cout << "Workspace: " << project.getWorkspaceName() << '\n';

    return 0;
}