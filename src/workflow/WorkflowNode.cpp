#include "WorkflowNode.h"

WorkflowNode::WorkflowNode(
    int id,
    const std::string& name,
    const std::string& description,
    double x,
    double y
)
    : id(id),
      name(name),
      description(description),
      x(x),
      y(y),
      state(WorkflowNodeState::Pending)
{
}

int WorkflowNode::getId() const { return id; }
const std::string& WorkflowNode::getName() const { return name; }
const std::string& WorkflowNode::getDescription() const { return description; }
double WorkflowNode::getX() const { return x; }
double WorkflowNode::getY() const { return y; }
WorkflowNodeState WorkflowNode::getState() const { return state; }
void WorkflowNode::setName(const std::string& value) { name = value; }
void WorkflowNode::setPosition(double newX, double newY) { x = newX; y = newY; }
void WorkflowNode::setState(WorkflowNodeState value) { state = value; }

std::string WorkflowNode::getStateName() const
{
    switch (state)
    {
        case WorkflowNodeState::Ready: return "Ready";
        case WorkflowNodeState::Completed: return "Completed";
        case WorkflowNodeState::Blocked: return "Blocked";
        default: return "Pending";
    }
}

WorkflowNodeKind InputWorkflowNode::getKind() const
{
    return WorkflowNodeKind::Input;
}

std::string InputWorkflowNode::getCategoryName() const { return "Input"; }
std::string InputWorkflowNode::getAccentColor() const { return "#2F80C1"; }

std::unique_ptr<WorkflowNode> InputWorkflowNode::clone() const
{
    return std::make_unique<InputWorkflowNode>(*this);
}

WorkflowNodeKind ProcessingWorkflowNode::getKind() const
{
    return WorkflowNodeKind::Processing;
}

std::string ProcessingWorkflowNode::getCategoryName() const
{
    return "Analysis";
}

std::string ProcessingWorkflowNode::getAccentColor() const
{
    return "#7B4AB5";
}

std::unique_ptr<WorkflowNode> ProcessingWorkflowNode::clone() const
{
    return std::make_unique<ProcessingWorkflowNode>(*this);
}

WorkflowNodeKind VisualizationWorkflowNode::getKind() const
{
    return WorkflowNodeKind::Visualization;
}

std::string VisualizationWorkflowNode::getCategoryName() const
{
    return "Visualization";
}

std::string VisualizationWorkflowNode::getAccentColor() const
{
    return "#D9822B";
}

std::unique_ptr<WorkflowNode> VisualizationWorkflowNode::clone() const
{
    return std::make_unique<VisualizationWorkflowNode>(*this);
}

WorkflowNodeKind OutputWorkflowNode::getKind() const
{
    return WorkflowNodeKind::Output;
}

std::string OutputWorkflowNode::getCategoryName() const { return "Output"; }
std::string OutputWorkflowNode::getAccentColor() const { return "#2D6A4F"; }

std::unique_ptr<WorkflowNode> OutputWorkflowNode::clone() const
{
    return std::make_unique<OutputWorkflowNode>(*this);
}

std::unique_ptr<WorkflowNode> createWorkflowNode(
    WorkflowNodeKind kind,
    int id,
    const std::string& name,
    const std::string& description,
    double x,
    double y
)
{
    switch (kind)
    {
        case WorkflowNodeKind::Input:
            return std::make_unique<InputWorkflowNode>(
                id, name, description, x, y
            );
        case WorkflowNodeKind::Visualization:
            return std::make_unique<VisualizationWorkflowNode>(
                id, name, description, x, y
            );
        case WorkflowNodeKind::Output:
            return std::make_unique<OutputWorkflowNode>(
                id, name, description, x, y
            );
        default:
            return std::make_unique<ProcessingWorkflowNode>(
                id, name, description, x, y
            );
    }
}
