#include "PCAResult.h"

#include <stdexcept>

PCAResult::PCAResult(
    const std::vector<std::string>& sampleNames,
    const std::vector<double>& pc1Scores,
    const std::vector<double>& pc2Scores,
    double pc1ExplainedVariance,
    double pc2ExplainedVariance
)
    : sampleNames(sampleNames),
      pc1Scores(pc1Scores),
      pc2Scores(pc2Scores),
      pc1ExplainedVariance(pc1ExplainedVariance),
      pc2ExplainedVariance(pc2ExplainedVariance)
{
    if (sampleNames.size() != pc1Scores.size()
        || sampleNames.size() != pc2Scores.size())
    {
        throw std::invalid_argument(
            "PCA result dimensions are inconsistent."
        );
    }
}

const std::vector<std::string>& PCAResult::getSampleNames() const
{
    return sampleNames;
}

const std::vector<double>& PCAResult::getPC1Scores() const
{
    return pc1Scores;
}

const std::vector<double>& PCAResult::getPC2Scores() const
{
    return pc2Scores;
}

double PCAResult::getPC1ExplainedVariance() const
{
    return pc1ExplainedVariance;
}

double PCAResult::getPC2ExplainedVariance() const
{
    return pc2ExplainedVariance;
}

std::size_t PCAResult::getSampleCount() const
{
    return sampleNames.size();
}
