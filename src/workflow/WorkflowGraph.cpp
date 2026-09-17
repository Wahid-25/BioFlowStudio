#include "WorkflowGraph.h"

#include <algorithm>
#include <functional>
#include <unordered_map>

int WorkflowGraph::addNode(
    WorkflowNodeKind kind,
    const std::string& name,
    const std::string& description,
    double x,
    double y
)
{
    int id = nextId++;
    nodes.push_back(createWorkflowNode(kind, id, name, description, x, y));
    return id;
}

bool WorkflowGraph::removeNode(int id)
{
    auto iterator = std::find_if(
        nodes.begin(), nodes.end(),
        [id](const auto& node) { return node->getId() == id; }
    );

    if (iterator == nodes.end())
    {
        return false;
    }

    nodes.erase(iterator);
    connections.erase(
        std::remove_if(
            connections.begin(), connections.end(),
            [id](const WorkflowConnection& connection)
            {
                return connection.sourceId == id
                    || connection.targetId == id;
            }
        ),
        connections.end()
    );
    return true;
}

bool WorkflowGraph::addConnection(int sourceId, int targetId)
{
    if (sourceId == targetId
        || getNode(sourceId) == nullptr
        || getNode(targetId) == nullptr)
    {
        return false;
    }

    auto duplicate = std::find_if(
        connections.begin(), connections.end(),
        [sourceId, targetId](const WorkflowConnection& connection)
        {
            return connection.sourceId == sourceId
                && connection.targetId == targetId;
        }
    );

    if (duplicate != connections.end())
    {
        return false;
    }

    connections.push_back({sourceId, targetId});

    std::string validation;

    if (!isValid(validation) && validation == "Workflow contains a cycle.")
    {
        connections.pop_back();
        return false;
    }

    return true;
}

bool WorkflowGraph::removeConnection(int sourceId, int targetId)
{
    std::size_t previousSize = connections.size();
    connections.erase(
        std::remove_if(
            connections.begin(), connections.end(),
            [sourceId, targetId](const WorkflowConnection& connection)
            {
                return connection.sourceId == sourceId
                    && connection.targetId == targetId;
            }
        ),
        connections.end()
    );
    return connections.size() != previousSize;
}

WorkflowNode* WorkflowGraph::getNode(int id)
{
    for (auto& node : nodes)
    {
        if (node->getId() == id)
        {
            return node.get();
        }
    }
    return nullptr;
}

const WorkflowNode* WorkflowGraph::getNode(int id) const
{
    for (const auto& node : nodes)
    {
        if (node->getId() == id)
        {
            return node.get();
        }
    }
    return nullptr;
}

const std::vector<std::unique_ptr<WorkflowNode>>&
WorkflowGraph::getNodes() const { return nodes; }

const std::vector<WorkflowConnection>&
WorkflowGraph::getConnections() const { return connections; }

bool WorkflowGraph::isValid(std::string& message) const
{
    if (nodes.empty())
    {
        message = "Workflow has no nodes.";
        return false;
    }

    std::unordered_map<int, int> visitState;

    for (const auto& node : nodes)
    {
        visitState[node->getId()] = 0;
    }

    std::function<bool(int)> hasCycle = [&](int nodeId)
    {
        visitState[nodeId] = 1;

        for (const WorkflowConnection& connection : connections)
        {
            if (connection.sourceId != nodeId)
            {
                continue;
            }

            if (visitState[connection.targetId] == 1)
            {
                return true;
            }

            if (visitState[connection.targetId] == 0
                && hasCycle(connection.targetId))
            {
                return true;
            }
        }

        visitState[nodeId] = 2;
        return false;
    };

    for (const auto& node : nodes)
    {
        if (visitState[node->getId()] == 0
            && hasCycle(node->getId()))
        {
            message = "Workflow contains a cycle.";
            return false;
        }
    }

    if (nodes.size() > 1 && connections.empty())
    {
        message = "Add connections between workflow nodes.";
        return false;
    }

    message = "Workflow is valid and contains no circular dependencies.";
    return true;
}

void WorkflowGraph::clear()
{
    nodes.clear();
    connections.clear();
    nextId = 1;
}
