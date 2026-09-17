#pragma once

#include <QPointF>
#include <QRectF>
#include <QString>
#include <QWidget>

#include <functional>

#include "workflow/WorkflowGraph.h"

class QColor;
class QPainter;

class WorkflowCanvasWidget : public QWidget
{
public:
    explicit WorkflowCanvasWidget(QWidget* parent = nullptr);

    void setGraph(WorkflowGraph graph);
    WorkflowGraph& getGraph();
    const WorkflowGraph& getGraph() const;

    int getSelectedNodeId() const;
    const WorkflowNode* getSelectedNode() const;

    int addNode(WorkflowNodeKind kind);
    void removeSelectedNode();
    void beginConnectionMode();
    void cancelConnectionMode();
    bool isConnecting() const;

    void setSelectionChangedCallback(
        std::function<void(const WorkflowNode*)> callback
    );
    void setGraphChangedCallback(std::function<void()> callback);
    void setMessageCallback(
        std::function<void(const QString&)> callback
    );

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    static constexpr double NodeWidth = 205.0;
    static constexpr double NodeHeight = 84.0;

    WorkflowGraph graph;
    int selectedNodeId = -1;
    int connectionSourceId = -1;
    bool connectionMode = false;
    bool dragging = false;
    QPointF dragOffset;

    std::function<void(const WorkflowNode*)> selectionChangedCallback;
    std::function<void()> graphChangedCallback;
    std::function<void(const QString&)> messageCallback;

    QRectF nodeRectangle(const WorkflowNode& node) const;
    int nodeAt(const QPointF& point) const;
    void selectNode(int id);
    QColor stateColor(WorkflowNodeState state) const;
    void drawConnection(
        QPainter& painter,
        const WorkflowConnection& connection
    ) const;
};
