#include "WorkflowCanvasWidget.h"

#include <QColor>
#include <QMouseEvent>
#include <QLineF>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QPolygonF>

#include <algorithm>
#include <cmath>
#include <utility>

WorkflowCanvasWidget::WorkflowCanvasWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumSize(920, 330);
    setMouseTracking(true);
    setStyleSheet("background-color: #F8FAFC;");
}

void WorkflowCanvasWidget::setGraph(WorkflowGraph newGraph)
{
    graph = std::move(newGraph);
    selectedNodeId = -1;
    connectionSourceId = -1;
    connectionMode = false;
    update();

    if (selectionChangedCallback)
    {
        selectionChangedCallback(nullptr);
    }

    if (graphChangedCallback)
    {
        graphChangedCallback();
    }
}

WorkflowGraph& WorkflowCanvasWidget::getGraph() { return graph; }
const WorkflowGraph& WorkflowCanvasWidget::getGraph() const { return graph; }
int WorkflowCanvasWidget::getSelectedNodeId() const { return selectedNodeId; }

const WorkflowNode* WorkflowCanvasWidget::getSelectedNode() const
{
    return graph.getNode(selectedNodeId);
}

int WorkflowCanvasWidget::addNode(WorkflowNodeKind kind)
{
    std::size_t index = graph.getNodes().size();
    double x = 45.0 + static_cast<double>(index % 4) * 235.0;
    double y = 65.0 + static_cast<double>(index / 4) * 145.0;
    std::string name;
    std::string description;

    switch (kind)
    {
        case WorkflowNodeKind::Input:
            name = "New Input";
            description = "Configure a biological input dataset.";
            break;
        case WorkflowNodeKind::Visualization:
            name = "New Visualization";
            description = "Display a graphical analysis result.";
            break;
        case WorkflowNodeKind::Output:
            name = "New Output";
            description = "Export results or generate a report.";
            break;
        default:
            name = "New Analysis";
            description = "Configure a bioinformatics processing step.";
            break;
    }

    int id = graph.addNode(kind, name, description, x, y);
    selectNode(id);
    update();

    if (graphChangedCallback)
    {
        graphChangedCallback();
    }

    return id;
}

void WorkflowCanvasWidget::removeSelectedNode()
{
    if (selectedNodeId < 0)
    {
        return;
    }

    graph.removeNode(selectedNodeId);
    selectedNodeId = -1;
    connectionSourceId = -1;
    update();

    if (selectionChangedCallback)
    {
        selectionChangedCallback(nullptr);
    }

    if (graphChangedCallback)
    {
        graphChangedCallback();
    }
}

void WorkflowCanvasWidget::beginConnectionMode()
{
    connectionMode = true;
    connectionSourceId = -1;

    if (messageCallback)
    {
        messageCallback("Connection mode: click the source node, then the target node.");
    }

    update();
}

void WorkflowCanvasWidget::cancelConnectionMode()
{
    connectionMode = false;
    connectionSourceId = -1;
    update();
}

bool WorkflowCanvasWidget::isConnecting() const { return connectionMode; }

void WorkflowCanvasWidget::setSelectionChangedCallback(
    std::function<void(const WorkflowNode*)> callback
)
{
    selectionChangedCallback = std::move(callback);
}

void WorkflowCanvasWidget::setGraphChangedCallback(
    std::function<void()> callback
)
{
    graphChangedCallback = std::move(callback);
}

void WorkflowCanvasWidget::setMessageCallback(
    std::function<void(const QString&)> callback
)
{
    messageCallback = std::move(callback);
}

QRectF WorkflowCanvasWidget::nodeRectangle(const WorkflowNode& node) const
{
    return QRectF(node.getX(), node.getY(), NodeWidth, NodeHeight);
}

int WorkflowCanvasWidget::nodeAt(const QPointF& point) const
{
    const auto& nodes = graph.getNodes();

    for (auto iterator = nodes.rbegin(); iterator != nodes.rend(); ++iterator)
    {
        if (nodeRectangle(**iterator).contains(point))
        {
            return (*iterator)->getId();
        }
    }

    return -1;
}

void WorkflowCanvasWidget::selectNode(int id)
{
    selectedNodeId = id;

    if (selectionChangedCallback)
    {
        selectionChangedCallback(graph.getNode(id));
    }
}

QColor WorkflowCanvasWidget::stateColor(WorkflowNodeState state) const
{
    switch (state)
    {
        case WorkflowNodeState::Ready: return QColor("#D79922");
        case WorkflowNodeState::Completed: return QColor("#2D8A57");
        case WorkflowNodeState::Blocked: return QColor("#C84646");
        default: return QColor("#8796A5");
    }
}

void WorkflowCanvasWidget::drawConnection(
    QPainter& painter,
    const WorkflowConnection& connection
) const
{
    const WorkflowNode* source = graph.getNode(connection.sourceId);
    const WorkflowNode* target = graph.getNode(connection.targetId);

    if (source == nullptr || target == nullptr)
    {
        return;
    }

    QRectF sourceRect = nodeRectangle(*source);
    QRectF targetRect = nodeRectangle(*target);
    QPointF start = sourceRect.center();
    QPointF end = targetRect.center();

    QLineF direction(start, end);

    if (direction.length() < 1.0)
    {
        return;
    }

    double angle = std::atan2(-direction.dy(), direction.dx());
    QPointF arrowPoint = end;
    double halfWidth = NodeWidth / 2.0;
    double halfHeight = NodeHeight / 2.0;
    double scaleX = std::abs(std::cos(angle)) > 0.001
        ? halfWidth / std::abs(std::cos(angle)) : 1.0e9;
    double scaleY = std::abs(std::sin(angle)) > 0.001
        ? halfHeight / std::abs(std::sin(angle)) : 1.0e9;
    double edgeDistance = std::min(scaleX, scaleY);
    arrowPoint = end - QPointF(
        std::cos(angle) * edgeDistance,
        -std::sin(angle) * edgeDistance
    );

    painter.setPen(QPen(QColor("#718497"), 2.2));
    painter.drawLine(start, arrowPoint);

    constexpr double arrowSize = 10.0;
    QPointF left = arrowPoint - QPointF(
        std::cos(angle - 0.55) * arrowSize,
        -std::sin(angle - 0.55) * arrowSize
    );
    QPointF right = arrowPoint - QPointF(
        std::cos(angle + 0.55) * arrowSize,
        -std::sin(angle + 0.55) * arrowSize
    );

    painter.setBrush(QColor("#718497"));
    painter.drawPolygon(QPolygonF({arrowPoint, left, right}));
}

void WorkflowCanvasWidget::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), QColor("#F8FAFC"));

    painter.setPen(QPen(QColor("#E4EAF0"), 1));

    for (int x = 0; x < width(); x += 24)
    {
        for (int y = 0; y < height(); y += 24)
        {
            painter.drawPoint(x, y);
        }
    }

    for (const WorkflowConnection& connection : graph.getConnections())
    {
        drawConnection(painter, connection);
    }

    for (const auto& node : graph.getNodes())
    {
        QRectF box = nodeRectangle(*node);
        QColor accent(QString::fromStdString(node->getAccentColor()));

        painter.setPen(QPen(
            node->getId() == selectedNodeId
                ? QColor("#0B66C3") : QColor("#C7D2DC"),
            node->getId() == selectedNodeId ? 3.0 : 1.5
        ));
        painter.setBrush(Qt::white);
        painter.drawRoundedRect(box, 10, 10);

        painter.setPen(Qt::NoPen);
        painter.setBrush(accent);
        painter.drawRoundedRect(
            QRectF(box.left(), box.top(), 9, box.height()),
            5, 5
        );

        painter.setPen(QColor("#163A5F"));
        QFont titleFont = painter.font();
        titleFont.setBold(true);
        titleFont.setPointSize(10);
        painter.setFont(titleFont);
        painter.drawText(
            QRectF(box.left() + 18, box.top() + 10, box.width() - 30, 24),
            Qt::AlignLeft | Qt::AlignVCenter,
            QString::fromStdString(node->getName())
        );

        QFont smallFont = painter.font();
        smallFont.setBold(false);
        smallFont.setPointSize(8);
        painter.setFont(smallFont);
        painter.setPen(QColor("#536A7B"));
        painter.drawText(
            QRectF(box.left() + 18, box.top() + 36, box.width() - 100, 35),
            Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap,
            QString::fromStdString(node->getCategoryName())
        );

        QRectF stateBox(box.right() - 82, box.bottom() - 30, 70, 20);
        painter.setPen(Qt::NoPen);
        painter.setBrush(stateColor(node->getState()));
        painter.drawRoundedRect(stateBox, 8, 8);
        painter.setPen(Qt::white);
        painter.drawText(
            stateBox,
            Qt::AlignCenter,
            QString::fromStdString(node->getStateName())
        );

        if (connectionMode && node->getId() == connectionSourceId)
        {
            painter.setBrush(QColor("#0B66C3"));
            painter.drawEllipse(box.topRight() + QPointF(-10, 10), 6, 6);
        }
    }
}

void WorkflowCanvasWidget::mousePressEvent(QMouseEvent* event)
{
    int clickedId = nodeAt(event->position());

    if (connectionMode)
    {
        if (clickedId < 0)
        {
            return;
        }

        if (connectionSourceId < 0)
        {
            connectionSourceId = clickedId;
            selectNode(clickedId);

            if (messageCallback)
            {
                messageCallback("Source selected. Now click the target node.");
            }
        }
        else
        {
            bool connected = graph.addConnection(
                connectionSourceId,
                clickedId
            );

            if (messageCallback)
            {
                messageCallback(
                    connected
                        ? "Connection created successfully."
                        : "Connection rejected: duplicate, self-link or cycle."
                );
            }

            connectionMode = false;
            connectionSourceId = -1;

            if (connected && graphChangedCallback)
            {
                graphChangedCallback();
            }
        }

        update();
        return;
    }

    selectNode(clickedId);

    if (clickedId >= 0)
    {
        const WorkflowNode* node = graph.getNode(clickedId);
        dragOffset = event->position()
            - QPointF(node->getX(), node->getY());
        dragging = true;
    }

    update();
}

void WorkflowCanvasWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (!dragging || selectedNodeId < 0)
    {
        return;
    }

    WorkflowNode* node = graph.getNode(selectedNodeId);
    QPointF requested = event->position() - dragOffset;
    double x = std::clamp(requested.x(), 5.0, width() - NodeWidth - 5.0);
    double y = std::clamp(requested.y(), 5.0, height() - NodeHeight - 5.0);
    node->setPosition(x, y);
    update();
}

void WorkflowCanvasWidget::mouseReleaseEvent(QMouseEvent*)
{
    if (dragging && graphChangedCallback)
    {
        graphChangedCallback();
    }

    dragging = false;
}
