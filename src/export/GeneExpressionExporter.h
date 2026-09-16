#pragma once

#include <string>
#include <vector>

#include "gene_expression/DifferentialExpressionResult.h"
#include "gene_expression/ExpressionDataset.h"
#include "gene_expression/SampleGrouping.h"

class GeneExpressionExporter
{
public:
    void exportResultsCSV(
        const std::string& filePath,
        const std::vector<DifferentialExpressionResult>& results
    ) const;

    void exportNormalizedMatrixCSV(
        const std::string& filePath,
        const ExpressionDataset& dataset,
        const std::vector<std::vector<double>>& normalizedValues,
        const SampleGrouping& grouping
    ) const;

    void exportAnalysisSummary(
        const std::string& filePath,
        const ExpressionDataset& dataset,
        const std::vector<DifferentialExpressionResult>& results,
        const SampleGrouping& grouping,
        const std::string& normalizationName,
        double adjustedPValueThreshold,
        double minimumAbsoluteLog2FoldChange
    ) const;

private:
    std::string escapeCSV(const std::string& value) const;
    std::string sampleGroupName(SampleGroup group) const;
};
