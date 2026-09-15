#pragma once

#include "DifferentialExpressionAnalyzer.h"

class WelchTTestAnalyzer
    : public DifferentialExpressionAnalyzer
{
public:
    std::string getName() const override;

    std::vector<
        DifferentialExpressionResult
    > analyze(
        const ExpressionDataset& dataset,
        const std::vector<std::vector<double>>&
            normalizedValues,
        const SampleGrouping& grouping
    ) const override;

private:
    std::vector<double> extractValues(
        const std::vector<double>& row,
        const std::vector<std::size_t>& indices
    ) const;
};