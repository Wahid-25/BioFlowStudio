#include "PhylogeneticTreeWidget.h"

#include <QFont>
#include <QPainter>
#include <QPaintEvent>
#include <QPen>
#include <QPointF>
#include <QString>

#include <algorithm>
#include <functional>
#include <map>

PhylogeneticTreeWidget::PhylogeneticTreeWidget(
    QWidget* parent
)
    : QWidget(parent)
{
    setMinimumHeight(180);
    setAutoFillBackground(true);
}

void PhylogeneticTreeWidget::setTree(
    const UPGMATree* newTree
)
{
    tree = newTree;
    update();
}

void PhylogeneticTreeWidget::clearTree()
{
    tree = nullptr;
    update();
}

void PhylogeneticTreeWidget::paintEvent(
    QPaintEvent* event
)
{
    QWidget::paintEvent(event);

    QPainter painter(this);

    painter.setRenderHint(
        QPainter::Antialiasing,
        true
    );

    painter.fillRect(
        rect(),
        QColor("#FFFFFF")
    );

    if (!tree || tree->isEmpty())
    {
        painter.setPen(QColor("#6C7A89"));

        painter.drawText(
            rect(),
            Qt::AlignCenter,
            "Generate a UPGMA tree to display "
            "the graphical result."
        );

        return;
    }

    const UPGMATree::Node* root =
        tree->getRoot();

    int leafCount = 0;

    std::function<void(const UPGMATree::Node*)>
        countLeaves;

    countLeaves =
        [&countLeaves, &leafCount](
            const UPGMATree::Node* node
        )
        {
            if (!node)
            {
                return;
            }

            if (node->isLeaf())
            {
                ++leafCount;
                return;
            }

            countLeaves(node->left.get());
            countLeaves(node->right.get());
        };

    countLeaves(root);

    double maximumHeight = root->height;

    if (maximumHeight <= 0.0)
    {
        maximumHeight = 1.0;
    }

    const double leftMargin = 45.0;
    const double rightMargin = 150.0;
    const double topMargin = 30.0;
    const double bottomMargin = 25.0;

    double usableWidth =
        width() - leftMargin - rightMargin;

    double usableHeight =
        height() - topMargin - bottomMargin;

    usableWidth = std::max(usableWidth, 100.0);
    usableHeight = std::max(usableHeight, 50.0);

    double leafSpacing =
        leafCount > 1
            ? usableHeight /
                static_cast<double>(leafCount - 1)
            : usableHeight / 2.0;

    int currentLeaf = 0;

    std::map<
        const UPGMATree::Node*,
        QPointF
    > positions;

    std::function<QPointF(const UPGMATree::Node*)>
        calculatePosition;

    calculatePosition =
        [&](const UPGMATree::Node* node)
        {
            double xPosition =
                leftMargin
                + (
                    (maximumHeight - node->height)
                    / maximumHeight
                ) * usableWidth;

            double yPosition = 0.0;

            if (node->isLeaf())
            {
                if (leafCount == 1)
                {
                    yPosition =
                        topMargin + usableHeight / 2.0;
                }
                else
                {
                    yPosition =
                        topMargin
                        + currentLeaf * leafSpacing;
                }

                ++currentLeaf;
            }
            else
            {
                QPointF leftPosition =
                    calculatePosition(
                        node->left.get()
                    );

                QPointF rightPosition =
                    calculatePosition(
                        node->right.get()
                    );

                yPosition =
                    (
                        leftPosition.y()
                        + rightPosition.y()
                    ) / 2.0;
            }

            QPointF position(
                xPosition,
                yPosition
            );

            positions[node] = position;

            return position;
        };

    calculatePosition(root);

    QPen branchPen(
        QColor("#245C8A"),
        2.0
    );

    painter.setPen(branchPen);

    std::function<void(const UPGMATree::Node*)>
        drawNode;

    drawNode =
        [&](const UPGMATree::Node* node)
        {
            QPointF nodePosition =
                positions.at(node);

            if (!node->isLeaf())
            {
                const UPGMATree::Node* leftChild =
                    node->left.get();

                const UPGMATree::Node* rightChild =
                    node->right.get();

                QPointF leftPosition =
                    positions.at(leftChild);

                QPointF rightPosition =
                    positions.at(rightChild);

                painter.setPen(branchPen);

                painter.drawLine(
                    QPointF(
                        nodePosition.x(),
                        leftPosition.y()
                    ),
                    QPointF(
                        nodePosition.x(),
                        rightPosition.y()
                    )
                );

                painter.drawLine(
                    QPointF(
                        nodePosition.x(),
                        leftPosition.y()
                    ),
                    leftPosition
                );

                painter.drawLine(
                    QPointF(
                        nodePosition.x(),
                        rightPosition.y()
                    ),
                    rightPosition
                );

                QFont branchFont = painter.font();
                branchFont.setPointSize(8);
                painter.setFont(branchFont);
                painter.setPen(QColor("#52697D"));

                double leftBranchLength =
                    std::max(
                        0.0,
                        node->height
                        - leftChild->height
                    );

                double rightBranchLength =
                    std::max(
                        0.0,
                        node->height
                        - rightChild->height
                    );

                painter.drawText(
                    QPointF(
                        (
                            nodePosition.x()
                            + leftPosition.x()
                        ) / 2.0 - 15.0,
                        leftPosition.y() - 4.0
                    ),
                    QString::number(
                        leftBranchLength,
                        'f',
                        3
                    )
                );

                painter.drawText(
                    QPointF(
                        (
                            nodePosition.x()
                            + rightPosition.x()
                        ) / 2.0 - 15.0,
                        rightPosition.y() - 4.0
                    ),
                    QString::number(
                        rightBranchLength,
                        'f',
                        3
                    )
                );

                drawNode(leftChild);
                drawNode(rightChild);
            }
            else
            {
                painter.setBrush(
                    QColor("#2D6A4F")
                );

                painter.setPen(Qt::NoPen);

                painter.drawEllipse(
                    nodePosition,
                    4.0,
                    4.0
                );

                QFont labelFont = painter.font();
                labelFont.setPointSize(9);
                labelFont.setBold(true);

                painter.setFont(labelFont);
                painter.setPen(QColor("#163A5F"));

                painter.drawText(
                    QPointF(
                        nodePosition.x() + 9.0,
                        nodePosition.y() + 5.0
                    ),
                    QString::fromStdString(
                        node->name
                    )
                );
            }
        };

    drawNode(root);

    painter.setBrush(QColor("#163A5F"));
    painter.setPen(Qt::NoPen);

    painter.drawEllipse(
        positions.at(root),
        5.0,
        5.0
    );
}