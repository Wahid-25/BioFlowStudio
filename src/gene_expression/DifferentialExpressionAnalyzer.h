#pragma once

#include <string>
#include <vector>

#include "DifferentialExpressionResult.h"
#include "ExpressionDataset.h"
#include "SampleGrouping.h"

class DifferentialExpressionAnalyzer
{
public:
    virtual ~DifferentialExpressionAnalyzer() = default;

    virtual std::string getName() const = 0;

    virtual std::vector<
        DifferentialExpressionResult
    > analyze(
        const ExpressionDataset& dataset,
        const std::vector<std::vector<double>>&
            normalizedValues,
        const SampleGrouping& grouping
    ) const = 0;
};