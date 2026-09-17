#include "PCAPlotWidget.h"

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
#include <QToolTip>

#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>

namespace
{
void configureSeries(
    QScatterSeries* series,
    const QString& name,
    const QColor& color
)
{
    series->setName(name);
    series->setMarkerSize(16.0);
    series->setColor(color);
    series->setBorderColor(color.darker(135));
}

void connectSampleTooltips(
    PCAPlotWidget* widget,
    QScatterSeries* series,
    std::vector<QString> sampleNames
)
{
    QObject::connect(
        series,
        &QScatterSeries::hovered,
        widget,
        [series, sampleNames = std::move(sampleNames)](
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

            QString sampleName =
                index >= 0
                && static_cast<std::size_t>(index) < sampleNames.size()
                    ? sampleNames.at(static_cast<std::size_t>(index))
                    : QString("Sample");

            QToolTip::showText(
                QCursor::pos(),
                QString("%1\nPC1: %2\nPC2: %3")
                    .arg(sampleName)
                    .arg(point.x(), 0, 'f', 4)
                    .arg(point.y(), 0, 'f', 4)
            );
        }
    );
}
}

PCAPlotWidget::PCAPlotWidget(QWidget* parent)
    : QChartView(parent)
{
    setRenderHint(QPainter::Antialiasing);
    setRubberBand(QChartView::RectangleRubberBand);
    clearPlot();
}

void PCAPlotWidget::clearPlot()
{
    QChart* emptyChart = new QChart;
    emptyChart->setTitle(
        "Run differential expression analysis to create a PCA plot"
    );
    emptyChart->setBackgroundBrush(QColor("#FFFFFF"));
    emptyChart->legend()->hide();
    setChart(emptyChart);
}

void PCAPlotWidget::setResult(
    const PCAResult& result,
    const SampleGrouping& grouping
)
{
    if (result.getSampleCount() == 0)
    {
        clearPlot();
        return;
    }

    QChart* pcaChart = new QChart;
    pcaChart->setTitle(
        "PCA: Sample Clustering by Expression Profile"
    );
    pcaChart->setBackgroundBrush(QColor("#FFFFFF"));
    pcaChart->setAnimationOptions(QChart::NoAnimation);

    QScatterSeries* controlSeries = new QScatterSeries;
    QScatterSeries* treatmentSeries = new QScatterSeries;
    QScatterSeries* unassignedSeries = new QScatterSeries;

    configureSeries(controlSeries, "Control", QColor("#2F80C1"));
    configureSeries(treatmentSeries, "Treatment", QColor("#E67E22"));
    configureSeries(unassignedSeries, "Unassigned", QColor("#7F8C8D"));

    std::vector<QString> controlNames;
    std::vector<QString> treatmentNames;
    std::vector<QString> unassignedNames;

    const auto& names = result.getSampleNames();
    const auto& pc1 = result.getPC1Scores();
    const auto& pc2 = result.getPC2Scores();

    double minimumX = pc1.front();
    double maximumX = pc1.front();
    double minimumY = pc2.front();
    double maximumY = pc2.front();

    for (std::size_t sample = 0;
         sample < result.getSampleCount();
         ++sample)
    {
        QPointF point(pc1.at(sample), pc2.at(sample));
        QString sampleName = QString::fromStdString(names.at(sample));

        minimumX = std::min(minimumX, point.x());
        maximumX = std::max(maximumX, point.x());
        minimumY = std::min(minimumY, point.y());
        maximumY = std::max(maximumY, point.y());

        SampleGroup group = grouping.getGroup(sample);

        if (group == SampleGroup::Control)
        {
            controlSeries->append(point);
            controlNames.push_back(sampleName);
        }
        else if (group == SampleGroup::Treatment)
        {
            treatmentSeries->append(point);
            treatmentNames.push_back(sampleName);
        }
        else
        {
            unassignedSeries->append(point);
            unassignedNames.push_back(sampleName);
        }
    }

    pcaChart->addSeries(controlSeries);
    pcaChart->addSeries(treatmentSeries);
    pcaChart->addSeries(unassignedSeries);

    double xSpan = maximumX - minimumX;
    double ySpan = maximumY - minimumY;
    double xPadding = std::max(0.5, xSpan * 0.18);
    double yPadding = std::max(0.5, ySpan * 0.18);

    QValueAxis* horizontalAxis = new QValueAxis;
    horizontalAxis->setTitleText(
        QString("PC1 (%1% variance)")
            .arg(result.getPC1ExplainedVariance(), 0, 'f', 1)
    );
    horizontalAxis->setRange(
        minimumX - xPadding,
        maximumX + xPadding
    );
    horizontalAxis->setLabelFormat("%.2f");
    horizontalAxis->setTickCount(7);

    QValueAxis* verticalAxis = new QValueAxis;
    verticalAxis->setTitleText(
        QString("PC2 (%1% variance)")
            .arg(result.getPC2ExplainedVariance(), 0, 'f', 1)
    );
    verticalAxis->setRange(
        minimumY - yPadding,
        maximumY + yPadding
    );
    verticalAxis->setLabelFormat("%.2f");
    verticalAxis->setTickCount(7);

    pcaChart->addAxis(horizontalAxis, Qt::AlignBottom);
    pcaChart->addAxis(verticalAxis, Qt::AlignLeft);

    for (QScatterSeries* series :
         {controlSeries, treatmentSeries, unassignedSeries})
    {
        series->attachAxis(horizontalAxis);
        series->attachAxis(verticalAxis);
    }

    QPen originPen(QColor("#AAB4BE"));
    originPen.setStyle(Qt::DashLine);

    QLineSeries* horizontalOrigin = new QLineSeries;
    horizontalOrigin->append(minimumX - xPadding, 0.0);
    horizontalOrigin->append(maximumX + xPadding, 0.0);
    horizontalOrigin->setPen(originPen);

    QLineSeries* verticalOrigin = new QLineSeries;
    verticalOrigin->append(0.0, minimumY - yPadding);
    verticalOrigin->append(0.0, maximumY + yPadding);
    verticalOrigin->setPen(originPen);

    pcaChart->addSeries(horizontalOrigin);
    pcaChart->addSeries(verticalOrigin);

    for (QLineSeries* series : {horizontalOrigin, verticalOrigin})
    {
        series->attachAxis(horizontalAxis);
        series->attachAxis(verticalAxis);
        pcaChart->legend()->markers(series).first()->setVisible(false);
    }

    pcaChart->legend()->setAlignment(Qt::AlignBottom);

    connectSampleTooltips(this, controlSeries, controlNames);
    connectSampleTooltips(this, treatmentSeries, treatmentNames);
    connectSampleTooltips(this, unassignedSeries, unassignedNames);

    setChart(pcaChart);
}
