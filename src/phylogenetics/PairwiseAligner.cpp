#include "PairwiseAligner.h"

#include <algorithm>
#include <stdexcept>
#include <vector>

std::string HammingAligner::getName() const
{
    return "Hamming Distance";
}

double HammingAligner::calculateDistance(
    const Sequence& first,
    const Sequence& second
) const
{
    if (first.getType() != second.getType())
    {
        throw std::invalid_argument(
            "Cannot compare nucleotide and protein sequences."
        );
    }

    const std::string& firstData =
        first.getSequenceData();

    const std::string& secondData =
        second.getSequenceData();

    if (firstData.length() != secondData.length())
    {
        throw std::invalid_argument(
            "Hamming distance requires sequences of equal length."
        );
    }

    if (firstData.empty())
    {
        return 0.0;
    }

    std::size_t differences = 0;

    for (std::size_t index = 0;
         index < firstData.length();
         ++index)
    {
        if (firstData[index] != secondData[index])
        {
            ++differences;
        }
    }

    return static_cast<double>(differences)
        / static_cast<double>(firstData.length());
}

NeedlemanWunschAligner::NeedlemanWunschAligner(
    int matchCost,
    int mismatchCost,
    int gapCost
)
    : matchCost(matchCost),
      mismatchCost(mismatchCost),
      gapCost(gapCost)
{
}

std::string NeedlemanWunschAligner::getName() const
{
    return "Needleman-Wunsch Global Distance";
}

double NeedlemanWunschAligner::calculateDistance(
    const Sequence& first,
    const Sequence& second
) const
{
    if (first.getType() != second.getType())
    {
        throw std::invalid_argument(
            "Cannot compare nucleotide and protein sequences."
        );
    }

    const std::string& firstData =
        first.getSequenceData();

    const std::string& secondData =
        second.getSequenceData();

    if (firstData.empty() && secondData.empty())
    {
        return 0.0;
    }

    std::vector<int> previousRow(
        secondData.length() + 1
    );

    std::vector<int> currentRow(
        secondData.length() + 1
    );

    for (std::size_t column = 0;
         column <= secondData.length();
         ++column)
    {
        previousRow[column] =
            static_cast<int>(column) * gapCost;
    }

    for (std::size_t row = 1;
         row <= firstData.length();
         ++row)
    {
        currentRow[0] =
            static_cast<int>(row) * gapCost;

        for (std::size_t column = 1;
             column <= secondData.length();
             ++column)
        {
            int substitutionCost =
                firstData[row - 1] ==
                        secondData[column - 1]
                    ? matchCost
                    : mismatchCost;

            int substitution =
                previousRow[column - 1]
                + substitutionCost;

            int deletion =
                previousRow[column] + gapCost;

            int insertion =
                currentRow[column - 1] + gapCost;

            currentRow[column] = std::min(
                {substitution, deletion, insertion}
            );
        }

        previousRow.swap(currentRow);
    }

    int totalDistance = previousRow.back();

    std::size_t normalizationLength = std::max(
        firstData.length(),
        secondData.length()
    );

    return static_cast<double>(totalDistance)
        / static_cast<double>(normalizationLength);
}