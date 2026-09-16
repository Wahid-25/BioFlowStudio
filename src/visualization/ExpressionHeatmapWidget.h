#pragma once

#include <QTableWidget>

#include <vector>

#include "gene_expression/DifferentialExpressionResult.h"
#include "gene_expression/ExpressionDataset.h"
#include "gene_expression/SampleGrouping.h"

class ExpressionHeatmapWidget : public QTableWidget
{
public:
    explicit ExpressionHeatmapWidget(QWidget* parent = nullptr);

    void setData(
        const ExpressionDataset& dataset,
        const std::vector<std::vector<double>>& normalizedValues,
        const std::vector<DifferentialExpressionResult>& results,
        const SampleGrouping& grouping,
        std::size_t maximumGenes = 50
    );

    void clearHeatmap();
};
