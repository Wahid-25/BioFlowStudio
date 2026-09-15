#pragma once

#include <QtCharts/QChartView>

#include <vector>

#include "gene_expression/DifferentialExpressionResult.h"

class VolcanoPlotWidget : public QChartView
{
public:
    explicit VolcanoPlotWidget(QWidget* parent = nullptr);

    void setResults(
        const std::vector<DifferentialExpressionResult>& results
    );

    void clearPlot();
};
