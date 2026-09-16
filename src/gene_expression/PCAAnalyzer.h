#pragma once

#include <vector>

#include "ExpressionDataset.h"
#include "PCAResult.h"

class PCAAnalyzer
{
public:
    PCAResult analyze(
        const ExpressionDataset& dataset,
        const std::vector<std::vector<double>>& normalizedValues
    ) const;

private:
    void jacobiEigenDecomposition(
        std::vector<std::vector<double>>& matrix,
        std::vector<double>& eigenvalues,
        std::vector<std::vector<double>>& eigenvectors
    ) const;
};
