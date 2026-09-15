#pragma once

#include <string>
#include <vector>

#include "ExpressionDataset.h"

class NormalizationStrategy
{
public:
    virtual ~NormalizationStrategy() = default;

    virtual std::string getName() const = 0;

    virtual std::vector<std::vector<double>> normalize(
        const ExpressionDataset& dataset
    ) const = 0;
};

class RawNormalization : public NormalizationStrategy
{
public:
    std::string getName() const override;

    std::vector<std::vector<double>> normalize(
        const ExpressionDataset& dataset
    ) const override;
};

class Log2Normalization : public NormalizationStrategy
{
public:
    std::string getName() const override;

    std::vector<std::vector<double>> normalize(
        const ExpressionDataset& dataset
    ) const override;
};

class ZScoreNormalization : public NormalizationStrategy
{
public:
    std::string getName() const override;

    std::vector<std::vector<double>> normalize(
        const ExpressionDataset& dataset
    ) const override;
};