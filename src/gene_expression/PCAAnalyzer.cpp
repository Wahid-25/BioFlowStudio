#include "PCAAnalyzer.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <stdexcept>
#include <utility>

PCAResult PCAAnalyzer::analyze(
    const ExpressionDataset& dataset,
    const std::vector<std::vector<double>>& normalizedValues
) const
{
    const std::size_t geneCount = dataset.getGeneCount();
    const std::size_t sampleCount = dataset.getSampleCount();

    if (geneCount < 2 || sampleCount < 3)
    {
        throw std::invalid_argument(
            "PCA requires at least two genes and three samples."
        );
    }

    if (normalizedValues.size() != geneCount)
    {
        throw std::invalid_argument(
            "PCA data does not match the number of genes."
        );
    }

    std::vector<std::vector<double>> centeredValues = normalizedValues;

    for (std::vector<double>& geneValues : centeredValues)
    {
        if (geneValues.size() != sampleCount)
        {
            throw std::invalid_argument(
                "PCA row does not match the number of samples."
            );
        }

        double mean = std::accumulate(
            geneValues.begin(),
            geneValues.end(),
            0.0
        ) / static_cast<double>(sampleCount);

        for (double& value : geneValues)
        {
            value -= mean;
        }
    }

    std::vector<std::vector<double>> sampleGramMatrix(
        sampleCount,
        std::vector<double>(sampleCount, 0.0)
    );

    const double denominator = static_cast<double>(sampleCount - 1);

    for (std::size_t left = 0; left < sampleCount; ++left)
    {
        for (std::size_t right = left; right < sampleCount; ++right)
        {
            double value = 0.0;

            for (std::size_t gene = 0; gene < geneCount; ++gene)
            {
                value += centeredValues.at(gene).at(left)
                       * centeredValues.at(gene).at(right);
            }

            value /= denominator;
            sampleGramMatrix.at(left).at(right) = value;
            sampleGramMatrix.at(right).at(left) = value;
        }
    }

    std::vector<double> eigenvalues;
    std::vector<std::vector<double>> eigenvectors;

    jacobiEigenDecomposition(
        sampleGramMatrix,
        eigenvalues,
        eigenvectors
    );

    std::vector<std::size_t> order(sampleCount);
    std::iota(order.begin(), order.end(), 0);

    std::sort(
        order.begin(),
        order.end(),
        [&eigenvalues](std::size_t left, std::size_t right)
        {
            return eigenvalues.at(left) > eigenvalues.at(right);
        }
    );

    double totalVariance = 0.0;

    for (double eigenvalue : eigenvalues)
    {
        totalVariance += std::max(0.0, eigenvalue);
    }

    if (totalVariance <= 1.0e-15)
    {
        throw std::runtime_error(
            "PCA cannot be calculated because the dataset has no variance."
        );
    }

    std::size_t firstComponent = order.at(0);
    std::size_t secondComponent = order.at(1);

    double firstEigenvalue = std::max(
        0.0,
        eigenvalues.at(firstComponent)
    );
    double secondEigenvalue = std::max(
        0.0,
        eigenvalues.at(secondComponent)
    );

    std::vector<double> pc1Scores(sampleCount);
    std::vector<double> pc2Scores(sampleCount);

    for (std::size_t sample = 0; sample < sampleCount; ++sample)
    {
        pc1Scores.at(sample) =
            eigenvectors.at(sample).at(firstComponent)
            * std::sqrt(firstEigenvalue);

        pc2Scores.at(sample) =
            eigenvectors.at(sample).at(secondComponent)
            * std::sqrt(secondEigenvalue);
    }

    return PCAResult(
        dataset.getSampleNames(),
        pc1Scores,
        pc2Scores,
        firstEigenvalue / totalVariance * 100.0,
        secondEigenvalue / totalVariance * 100.0
    );
}

void PCAAnalyzer::jacobiEigenDecomposition(
    std::vector<std::vector<double>>& matrix,
    std::vector<double>& eigenvalues,
    std::vector<std::vector<double>>& eigenvectors
) const
{
    const std::size_t size = matrix.size();

    eigenvectors.assign(
        size,
        std::vector<double>(size, 0.0)
    );

    for (std::size_t index = 0; index < size; ++index)
    {
        eigenvectors.at(index).at(index) = 1.0;
    }

    const std::size_t maximumIterations = 100 * size * size;
    constexpr double tolerance = 1.0e-12;

    for (std::size_t iteration = 0;
         iteration < maximumIterations;
         ++iteration)
    {
        std::size_t pivotRow = 0;
        std::size_t pivotColumn = 1;
        double largestOffDiagonal = 0.0;

        for (std::size_t row = 0; row < size; ++row)
        {
            for (std::size_t column = row + 1;
                 column < size;
                 ++column)
            {
                double magnitude = std::abs(
                    matrix.at(row).at(column)
                );

                if (magnitude > largestOffDiagonal)
                {
                    largestOffDiagonal = magnitude;
                    pivotRow = row;
                    pivotColumn = column;
                }
            }
        }

        if (largestOffDiagonal < tolerance)
        {
            break;
        }

        double diagonalDifference =
            matrix.at(pivotColumn).at(pivotColumn)
            - matrix.at(pivotRow).at(pivotRow);

        double angle = 0.5 * std::atan2(
            2.0 * matrix.at(pivotRow).at(pivotColumn),
            diagonalDifference
        );

        double cosine = std::cos(angle);
        double sine = std::sin(angle);

        for (std::size_t index = 0; index < size; ++index)
        {
            if (index == pivotRow || index == pivotColumn)
            {
                continue;
            }

            double rowValue = matrix.at(index).at(pivotRow);
            double columnValue = matrix.at(index).at(pivotColumn);

            matrix.at(index).at(pivotRow) =
                cosine * rowValue - sine * columnValue;
            matrix.at(pivotRow).at(index) =
                matrix.at(index).at(pivotRow);

            matrix.at(index).at(pivotColumn) =
                sine * rowValue + cosine * columnValue;
            matrix.at(pivotColumn).at(index) =
                matrix.at(index).at(pivotColumn);
        }

        double rowDiagonal = matrix.at(pivotRow).at(pivotRow);
        double columnDiagonal = matrix.at(pivotColumn).at(pivotColumn);
        double pivotValue = matrix.at(pivotRow).at(pivotColumn);

        matrix.at(pivotRow).at(pivotRow) =
            cosine * cosine * rowDiagonal
            - 2.0 * sine * cosine * pivotValue
            + sine * sine * columnDiagonal;

        matrix.at(pivotColumn).at(pivotColumn) =
            sine * sine * rowDiagonal
            + 2.0 * sine * cosine * pivotValue
            + cosine * cosine * columnDiagonal;

        matrix.at(pivotRow).at(pivotColumn) = 0.0;
        matrix.at(pivotColumn).at(pivotRow) = 0.0;

        for (std::size_t row = 0; row < size; ++row)
        {
            double first = eigenvectors.at(row).at(pivotRow);
            double second = eigenvectors.at(row).at(pivotColumn);

            eigenvectors.at(row).at(pivotRow) =
                cosine * first - sine * second;
            eigenvectors.at(row).at(pivotColumn) =
                sine * first + cosine * second;
        }
    }

    eigenvalues.resize(size);

    for (std::size_t index = 0; index < size; ++index)
    {
        eigenvalues.at(index) = matrix.at(index).at(index);
    }
}
