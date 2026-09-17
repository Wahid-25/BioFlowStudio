#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "WorkflowNode.h"

struct WorkflowConnection
{
    int sourceId;
    int targetId;
};

class WorkflowGraph
{
private:
    std::vector<std::unique_ptr<WorkflowNode>> nodes;
    std::vector<WorkflowConnection> connections;
    int nextId = 1;

public:
    WorkflowGraph() = default;
    WorkflowGraph(WorkflowGraph&&) noexcept = default;
    WorkflowGraph& operator=(WorkflowGraph&&) noexcept = default;
    WorkflowGraph(const WorkflowGraph&) = delete;
    WorkflowGraph& operator=(const WorkflowGraph&) = delete;

    int addNode(
        WorkflowNodeKind kind,
        const std::string& name,
        const std::string& description,
        double x,
        double y
    );

    bool removeNode(int id);
    bool addConnection(int sourceId, int targetId);
    bool removeConnection(int sourceId, int targetId);

    WorkflowNode* getNode(int id);
    const WorkflowNode* getNode(int id) const;

    const std::vector<std::unique_ptr<WorkflowNode>>& getNodes() const;
    const std::vector<WorkflowConnection>& getConnections() const;

    bool isValid(std::string& message) const;
    void clear();
};
