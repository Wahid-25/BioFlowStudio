#include "NormalizationStrategy.h"

#include <cmath>

std::string RawNormalization::getName() const
{
    return "Raw Values";
}

std::vector<std::vector<double>>
RawNormalization::normalize(
    const ExpressionDataset& dataset
) const
{
    return dataset.getValues();
}

std::string Log2Normalization::getName() const
{
    return "Log2(x + 1)";
}

std::vector<std::vector<double>>
Log2Normalization::normalize(
    const ExpressionDataset& dataset
) const
{
    std::vector<std::vector<double>> result =
        dataset.getValues();

    for (auto& row : result)
    {
        for (double& value : row)
        {
            value = std::log2(value + 1.0);
        }
    }

    return result;
}

std::string ZScoreNormalization::getName() const
{
    return "Z-Score Per Gene";
}

std::vector<std::vector<double>>
ZScoreNormalization::normalize(
    const ExpressionDataset& dataset
) const
{
    std::vector<std::vector<double>> result =
        dataset.getValues();

    for (auto& row : result)
    {
        if (row.empty())
        {
            continue;
        }

        double total = 0.0;

        for (double value : row)
        {
            total += value;
        }

        double mean =
            total / static_cast<double>(row.size());

        double squaredDifferenceTotal = 0.0;

        for (double value : row)
        {
            double difference = value - mean;

            squaredDifferenceTotal +=
                difference * difference;
        }

        double variance =
            squaredDifferenceTotal
            / static_cast<double>(row.size());

        double standardDeviation =
            std::sqrt(variance);

        for (double& value : row)
        {
            if (standardDeviation == 0.0)
            {
                value = 0.0;
            }
            else
            {
                value =
                    (value - mean)
                    / standardDeviation;
            }
        }
    }

    return result;
}