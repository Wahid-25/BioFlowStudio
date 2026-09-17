#pragma once

#include <QtCharts/QChartView>

#include <vector>

#include "gene_expression/EnrichmentResult.h"

class EnrichmentBarChartWidget : public QChartView
{
public:
    explicit EnrichmentBarChartWidget(QWidget* parent = nullptr);

    void setResults(
        const std::vector<EnrichmentResult>& results,
        std::size_t maximumPathways = 10
    );

    void clearResults();
};
