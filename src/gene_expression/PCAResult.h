#pragma once

#include <cstddef>
#include <string>
#include <vector>

class PCAResult
{
private:
    std::vector<std::string> sampleNames;
    std::vector<double> pc1Scores;
    std::vector<double> pc2Scores;
    double pc1ExplainedVariance;
    double pc2ExplainedVariance;

public:
    PCAResult(
        const std::vector<std::string>& sampleNames,
        const std::vector<double>& pc1Scores,
        const std::vector<double>& pc2Scores,
        double pc1ExplainedVariance,
        double pc2ExplainedVariance
    );

    const std::vector<std::string>& getSampleNames() const;
    const std::vector<double>& getPC1Scores() const;
    const std::vector<double>& getPC2Scores() const;

    double getPC1ExplainedVariance() const;
    double getPC2ExplainedVariance() const;
    std::size_t getSampleCount() const;
};
