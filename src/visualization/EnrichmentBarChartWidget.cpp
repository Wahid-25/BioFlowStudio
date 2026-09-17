#include "EnrichmentBarChartWidget.h"

#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QBarSet>
#include <QtCharts/QChart>
#include <QtCharts/QHorizontalBarSeries>
#include <QtCharts/QLegend>
#include <QtCharts/QValueAxis>

#include <QColor>
#include <QPainter>
#include <QStringList>

#include <algorithm>
#include <cmath>

EnrichmentBarChartWidget::EnrichmentBarChartWidget(QWidget* parent)
    : QChartView(parent)
{
    setRenderHint(QPainter::Antialiasing);
    setMinimumHeight(240);
    clearResults();
}

void EnrichmentBarChartWidget::clearResults()
{
    QChart* emptyChart = new QChart;
    emptyChart->setTitle("Run enrichment analysis to display pathways");
    emptyChart->legend()->hide();
    emptyChart->setBackgroundBrush(QColor("#FFFFFF"));
    setChart(emptyChart);
}

void EnrichmentBarChartWidget::setResults(
    const std::vector<EnrichmentResult>& results,
    std::size_t maximumPathways
)
{
    if (results.empty())
    {
        clearResults();
        return;
    }

    std::size_t count = std::min(maximumPathways, results.size());
    QBarSet* significanceSet = new QBarSet("-log10 adjusted p-value");
    significanceSet->setColor(QColor("#2B7A78"));

    QStringList categories;
    double maximumValue = 1.0;

    for (std::size_t reverseIndex = count; reverseIndex > 0; --reverseIndex)
    {
        const EnrichmentResult& result = results.at(reverseIndex - 1);
        double adjusted = std::max(result.getAdjustedPValue(), 1.0e-300);
        double significance = -std::log10(adjusted);

        *significanceSet << significance;
        maximumValue = std::max(maximumValue, significance);
        categories << QString::fromStdString(result.getPathwayName());
    }

    QHorizontalBarSeries* series = new QHorizontalBarSeries;
    series->append(significanceSet);

    QChart* resultChart = new QChart;
    resultChart->addSeries(series);
    resultChart->setTitle("Top Enriched Pathways");
    resultChart->setAnimationOptions(QChart::SeriesAnimations);
    resultChart->setBackgroundBrush(QColor("#FFFFFF"));
    resultChart->legend()->hide();

    QBarCategoryAxis* categoryAxis = new QBarCategoryAxis;
    categoryAxis->append(categories);

    QValueAxis* valueAxis = new QValueAxis;
    valueAxis->setTitleText("Statistical significance (-log10 adjusted p)");
    valueAxis->setRange(0.0, maximumValue * 1.15);
    valueAxis->setLabelFormat("%.2f");

    resultChart->addAxis(categoryAxis, Qt::AlignLeft);
    resultChart->addAxis(valueAxis, Qt::AlignBottom);
    series->attachAxis(categoryAxis);
    series->attachAxis(valueAxis);

    setChart(resultChart);
}
