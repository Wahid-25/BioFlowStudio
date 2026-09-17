#pragma once

#include <memory>
#include <string>

enum class WorkflowNodeState
{
    Pending,
    Ready,
    Completed,
    Blocked
};

enum class WorkflowNodeKind
{
    Input,
    Processing,
    Visualization,
    Output
};

class WorkflowNode
{
private:
    int id;
    std::string name;
    std::string description;
    double x;
    double y;
    WorkflowNodeState state;

public:
    WorkflowNode(
        int id,
        const std::string& name,
        const std::string& description,
        double x,
        double y
    );

    virtual ~WorkflowNode() = default;

    int getId() const;
    const std::string& getName() const;
    const std::string& getDescription() const;
    double getX() const;
    double getY() const;
    WorkflowNodeState getState() const;

    void setName(const std::string& value);
    void setPosition(double x, double y);
    void setState(WorkflowNodeState value);

    std::string getStateName() const;

    virtual WorkflowNodeKind getKind() const = 0;
    virtual std::string getCategoryName() const = 0;
    virtual std::string getAccentColor() const = 0;
    virtual std::unique_ptr<WorkflowNode> clone() const = 0;
};

class InputWorkflowNode : public WorkflowNode
{
public:
    using WorkflowNode::WorkflowNode;
    WorkflowNodeKind getKind() const override;
    std::string getCategoryName() const override;
    std::string getAccentColor() const override;
    std::unique_ptr<WorkflowNode> clone() const override;
};

class ProcessingWorkflowNode : public WorkflowNode
{
public:
    using WorkflowNode::WorkflowNode;
    WorkflowNodeKind getKind() const override;
    std::string getCategoryName() const override;
    std::string getAccentColor() const override;
    std::unique_ptr<WorkflowNode> clone() const override;
};

class VisualizationWorkflowNode : public WorkflowNode
{
public:
    using WorkflowNode::WorkflowNode;
    WorkflowNodeKind getKind() const override;
    std::string getCategoryName() const override;
    std::string getAccentColor() const override;
    std::unique_ptr<WorkflowNode> clone() const override;
};

class OutputWorkflowNode : public WorkflowNode
{
public:
    using WorkflowNode::WorkflowNode;
    WorkflowNodeKind getKind() const override;
    std::string getCategoryName() const override;
    std::string getAccentColor() const override;
    std::unique_ptr<WorkflowNode> clone() const override;
};

std::unique_ptr<WorkflowNode> createWorkflowNode(
    WorkflowNodeKind kind,
    int id,
    const std::string& name,
    const std::string& description,
    double x,
    double y
);
