#include "DistanceMatrix.h"

#include <stdexcept>

void DistanceMatrix::calculate(
    const std::vector<std::unique_ptr<Sequence>>& sequences,
    const PairwiseAligner& aligner
)
{
    if (sequences.size() < 2)
    {
        throw std::invalid_argument(
            "At least two sequences are required."
        );
    }

    clear();

    algorithmName = aligner.getName();

    for (const auto& sequence : sequences)
    {
        if (!sequence)
        {
            throw std::invalid_argument(
                "A null sequence was found."
            );
        }

        labels.push_back(sequence->getIdentifier());
    }

    values.assign(
        sequences.size(),
        std::vector<double>(sequences.size(), 0.0)
    );

    for (std::size_t row = 0;
         row < sequences.size();
         ++row)
    {
        for (std::size_t column = row + 1;
             column < sequences.size();
             ++column)
        {
            double distance = aligner.calculateDistance(
                *sequences[row],
                *sequences[column]
            );

            values[row][column] = distance;
            values[column][row] = distance;
        }
    }
}

std::size_t DistanceMatrix::size() const
{
    return values.size();
}

double DistanceMatrix::getDistance(
    std::size_t row,
    std::size_t column
) const
{
    if (row >= values.size()
        || column >= values.size())
    {
        throw std::out_of_range(
            "Distance matrix index is outside its range."
        );
    }

    return values[row][column];
}

const std::vector<std::string>&
DistanceMatrix::getLabels() const
{
    return labels;
}

const std::vector<std::vector<double>>&
DistanceMatrix::getValues() const
{
    return values;
}

const std::string&
DistanceMatrix::getAlgorithmName() const
{
    return algorithmName;
}

void DistanceMatrix::clear()
{
    labels.clear();
    values.clear();
    algorithmName.clear();
}