#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "DifferentialExpressionResult.h"

enum class RegulationFilter
{
    All,
    Upregulated,
    Downregulated,
    NotSignificant
};

struct ExpressionFilterSettings
{
    std::string geneQuery;
    RegulationFilter regulationFilter = RegulationFilter::All;
    double adjustedPValueThreshold = 0.05;
    double minimumAbsoluteLog2FoldChange = 1.0;
    std::size_t maximumResults = 0;
};

class ExpressionFilterEngine
{
public:
    std::vector<DifferentialExpressionResult> classify(
        const std::vector<DifferentialExpressionResult>& results,
        const ExpressionFilterSettings& settings
    ) const;

    std::vector<DifferentialExpressionResult> filter(
        const std::vector<DifferentialExpressionResult>& classifiedResults,
        const ExpressionFilterSettings& settings
    ) const;

private:
    bool matchesRegulation(
        RegulationStatus status,
        RegulationFilter filter
    ) const;

    std::string lowerCase(const std::string& text) const;
};
