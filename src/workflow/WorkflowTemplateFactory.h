#pragma once

#include "WorkflowGraph.h"

enum class WorkflowTemplateType
{
    Phylogenetic,
    GeneExpression
};

class WorkflowTemplateFactory
{
public:
    static WorkflowGraph create(WorkflowTemplateType type);

private:
    static WorkflowGraph createPhylogeneticWorkflow();
    static WorkflowGraph createExpressionWorkflow();
};
