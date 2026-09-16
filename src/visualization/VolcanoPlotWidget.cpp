#include "VolcanoPlotWidget.h"

#include <QtCharts/QChart>
#include <QtCharts/QLegend>
#include <QtCharts/QLegendMarker>
#include <QtCharts/QLineSeries>
#include <QtCharts/QScatterSeries>
#include <QtCharts/QValueAxis>

#include <QColor>
#include <QCursor>
#include <QPainter>
#include <QPen>
#include <QString>
#include <QToolTip>

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace
{
double significanceScore(double adjustedPValue)
{
    constexpr double minimumPValue = 1.0e-300;
    return -std::log10(
        std::max(adjustedPValue, minimumPValue)
    );
}

void configureScatterSeries(
    QScatterSeries* series,
    const QString& name,
    const QColor& color
)
{
    series->setName(name);
    series->setMarkerSize(11.0);
    series->setColor(color);
    series->setBorderColor(color.darker(120));
}

void connectTooltips(
    VolcanoPlotWidget* widget,
    QScatterSeries* series,
    std::vector<QString> labels
)
{
    QObject::connect(
        series,
        &QScatterSeries::hovered,
        widget,
        [series, labels = std::move(labels)](
            const QPointF& point,
            bool state
        )
        {
            if (!state)
            {
                QToolTip::hideText();
                return;
            }

            const QList<QPointF> points = series->points();
            int index = points.indexOf(point);

            QString geneName =
                index >= 0
                && static_cast<std::size_t>(index) < labels.size()
                    ? labels.at(static_cast<std::size_t>(index))
                    : QString("Gene");

            QToolTip::showText(
                QCursor::pos(),
                QString(
                    "%1\nLog2 fold change: %2\n"
                    "-log10 adjusted p-value: %3"
                )
                .arg(geneName)
                .arg(point.x(), 0, 'f', 3)
                .arg(point.y(), 0, 'f', 3)
            );
        }
    );
}
}

VolcanoPlotWidget::VolcanoPlotWidget(QWidget* parent)
    : QChartView(parent)
{
    setRenderHint(QPainter::Antialiasing);
    setRubberBand(QChartView::RectangleRubberBand);
    clearPlot();
}

void VolcanoPlotWidget::clearPlot()
{
    QChart* emptyChart = new QChart;
    emptyChart->setTitle(
        "Run a differential expression analysis to create a volcano plot"
    );
    emptyChart->setBackgroundBrush(QColor("#FFFFFF"));
    emptyChart->legend()->hide();
    setChart(emptyChart);
}

void VolcanoPlotWidget::setResults(
    const std::vector<DifferentialExpressionResult>& results,
    double adjustedPValueThreshold,
    double foldChangeThreshold
)
{
    if (results.empty())
    {
        clearPlot();
        return;
    }

    QChart* volcanoChart = new QChart;
    volcanoChart->setTitle(
        "Volcano Plot: Differential Gene Expression"
    );
    volcanoChart->setBackgroundBrush(QColor("#FFFFFF"));
    volcanoChart->setAnimationOptions(QChart::SeriesAnimations);

    QScatterSeries* upregulatedSeries = new QScatterSeries;
    QScatterSeries* downregulatedSeries = new QScatterSeries;
    QScatterSeries* unchangedSeries = new QScatterSeries;

    configureScatterSeries(
        upregulatedSeries,
        "Upregulated",
        QColor("#2E9D57")
    );
    configureScatterSeries(
        downregulatedSeries,
        "Downregulated",
        QColor("#D84A4A")
    );
    configureScatterSeries(
        unchangedSeries,
        "Not significant",
        QColor("#8A98A6")
    );

    std::vector<QString> upregulatedLabels;
    std::vector<QString> downregulatedLabels;
    std::vector<QString> unchangedLabels;

    double maximumAbsoluteFoldChange = 0.0;
    double maximumSignificance = 0.0;

    for (const DifferentialExpressionResult& result : results)
    {
        double foldChange = result.getLog2FoldChange();
        double significance = significanceScore(
            result.getAdjustedPValue()
        );

        maximumAbsoluteFoldChange = std::max(
            maximumAbsoluteFoldChange,
            std::abs(foldChange)
        );
        maximumSignificance = std::max(
            maximumSignificance,
            significance
        );

        QString geneName = QString::fromStdString(
            result.getGeneName()
        );

        if (result.getRegulationStatus()
            == RegulationStatus::Upregulated)
        {
            upregulatedSeries->append(foldChange, significance);
            upregulatedLabels.push_back(geneName);
        }
        else if (result.getRegulationStatus()
                 == RegulationStatus::Downregulated)
        {
            downregulatedSeries->append(foldChange, significance);
            downregulatedLabels.push_back(geneName);
        }
        else
        {
            unchangedSeries->append(foldChange, significance);
            unchangedLabels.push_back(geneName);
        }
    }

    volcanoChart->addSeries(unchangedSeries);
    volcanoChart->addSeries(upregulatedSeries);
    volcanoChart->addSeries(downregulatedSeries);

    double xLimit = std::max(
        2.0,
        maximumAbsoluteFoldChange * 1.15
    );
    double yLimit = std::max(
        2.0,
        maximumSignificance * 1.15
    );

    QValueAxis* horizontalAxis = new QValueAxis;
    horizontalAxis->setTitleText("Log2 Fold Change");
    horizontalAxis->setRange(-xLimit, xLimit);
    horizontalAxis->setLabelFormat("%.1f");
    horizontalAxis->setTickCount(9);

    QValueAxis* verticalAxis = new QValueAxis;
    verticalAxis->setTitleText("-log10 Adjusted P-value");
    verticalAxis->setRange(0.0, yLimit);
    verticalAxis->setLabelFormat("%.1f");
    verticalAxis->setTickCount(7);

    volcanoChart->addAxis(horizontalAxis, Qt::AlignBottom);
    volcanoChart->addAxis(verticalAxis, Qt::AlignLeft);

    for (QScatterSeries* series :
         {unchangedSeries, upregulatedSeries, downregulatedSeries})
    {
        series->attachAxis(horizontalAxis);
        series->attachAxis(verticalAxis);
    }

    QPen thresholdPen(QColor("#5D6D7E"));
    thresholdPen.setStyle(Qt::DashLine);
    thresholdPen.setWidth(2);

    QLineSeries* leftThreshold = new QLineSeries;
    leftThreshold->append(-foldChangeThreshold, 0.0);
    leftThreshold->append(-foldChangeThreshold, yLimit);
    leftThreshold->setPen(thresholdPen);

    QLineSeries* rightThreshold = new QLineSeries;
    rightThreshold->append(foldChangeThreshold, 0.0);
    rightThreshold->append(foldChangeThreshold, yLimit);
    rightThreshold->setPen(thresholdPen);

    QLineSeries* significanceThreshold = new QLineSeries;
    significanceThreshold->append(
        -xLimit,
        significanceScore(adjustedPValueThreshold)
    );
    significanceThreshold->append(
        xLimit,
        significanceScore(adjustedPValueThreshold)
    );
    significanceThreshold->setPen(thresholdPen);

    volcanoChart->addSeries(leftThreshold);
    volcanoChart->addSeries(rightThreshold);
    volcanoChart->addSeries(significanceThreshold);

    for (QLineSeries* series :
         {leftThreshold, rightThreshold, significanceThreshold})
    {
        series->attachAxis(horizontalAxis);
        series->attachAxis(verticalAxis);
        volcanoChart->legend()->markers(series).first()->setVisible(false);
    }

    volcanoChart->legend()->setAlignment(Qt::AlignBottom);

    connectTooltips(this, upregulatedSeries, upregulatedLabels);
    connectTooltips(this, downregulatedSeries, downregulatedLabels);
    connectTooltips(this, unchangedSeries, unchangedLabels);

    setChart(volcanoChart);
}
