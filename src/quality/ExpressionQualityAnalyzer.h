#pragma once

#include "DataQualityAnalyzer.h"
#include "gene_expression/ExpressionDataset.h"
#include "gene_expression/SampleGrouping.h"

class ExpressionQualityAnalyzer : public DataQualityAnalyzer
{
private:
    const ExpressionDataset& dataset;
    const SampleGrouping& grouping;

public:
    ExpressionQualityAnalyzer(
        const ExpressionDataset& dataset,
        const SampleGrouping& grouping
    );

    std::string getName() const override;
    QualityReport analyze() const override;
};
