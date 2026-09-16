#pragma once

#include <QtCharts/QChartView>

#include "gene_expression/PCAResult.h"
#include "gene_expression/SampleGrouping.h"

class PCAPlotWidget : public QChartView
{
public:
    explicit PCAPlotWidget(QWidget* parent = nullptr);

    void setResult(
        const PCAResult& result,
        const SampleGrouping& grouping
    );

    void clearPlot();
};
