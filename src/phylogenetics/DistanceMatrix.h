#pragma once

#include <memory>
#include <string>
#include <vector>

#include "PairwiseAligner.h"
#include "Sequence.h"

class DistanceMatrix
{
private:
    std::vector<std::string> labels;
    std::vector<std::vector<double>> values;
    std::string algorithmName;

public:
    void calculate(
        const std::vector<std::unique_ptr<Sequence>>& sequences,
        const PairwiseAligner& aligner
    );

    std::size_t size() const;

    double getDistance(
        std::size_t row,
        std::size_t column
    ) const;

    const std::vector<std::string>& getLabels() const;

    const std::vector<std::vector<double>>&
    getValues() const;

    const std::string& getAlgorithmName() const;

    void clear();
};