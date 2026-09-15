#include "StatisticsUtilities.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

double StatisticsUtilities::mean(
    const std::vector<double>& values
)
{
    if (values.empty())
    {
        throw std::invalid_argument(
            "Cannot calculate the mean of an empty group."
        );
    }

    double total = 0.0;

    for (double value : values)
    {
        total += value;
    }

    return total /
        static_cast<double>(values.size());
}

double StatisticsUtilities::sampleVariance(
    const std::vector<double>& values
)
{
    if (values.size() < 2)
    {
        throw std::invalid_argument(
            "At least two values are required "
            "to calculate sample variance."
        );
    }

    double groupMean = mean(values);
    double squaredDifferenceTotal = 0.0;

    for (double value : values)
    {
        double difference = value - groupMean;

        squaredDifferenceTotal +=
            difference * difference;
    }

    return squaredDifferenceTotal /
        static_cast<double>(
            values.size() - 1
        );
}

double StatisticsUtilities::welchPValue(
    const std::vector<double>& firstGroup,
    const std::vector<double>& secondGroup
)
{
    if (firstGroup.size() < 2
        || secondGroup.size() < 2)
    {
        throw std::invalid_argument(
            "Welch's t-test requires at least "
            "two values in each group."
        );
    }

    double firstMean = mean(firstGroup);
    double secondMean = mean(secondGroup);

    double firstVariance =
        sampleVariance(firstGroup);

    double secondVariance =
        sampleVariance(secondGroup);

    double firstSize =
        static_cast<double>(firstGroup.size());

    double secondSize =
        static_cast<double>(secondGroup.size());

    double firstVarianceTerm =
        firstVariance / firstSize;

    double secondVarianceTerm =
        secondVariance / secondSize;

    double standardErrorSquared =
        firstVarianceTerm
        + secondVarianceTerm;

    if (standardErrorSquared
        <= std::numeric_limits<double>::epsilon())
    {
        return std::abs(firstMean - secondMean)
                   <= std::numeric_limits<double>::epsilon()
            ? 1.0
            : 0.0;
    }

    double tStatistic =
        (firstMean - secondMean)
        / std::sqrt(standardErrorSquared);

    double degreesNumerator =
        standardErrorSquared
        * standardErrorSquared;

    double degreesDenominator =
        (
            firstVarianceTerm
            * firstVarianceTerm
        ) / (firstSize - 1.0)
        +
        (
            secondVarianceTerm
            * secondVarianceTerm
        ) / (secondSize - 1.0);

    if (degreesDenominator <= 0.0)
    {
        return 1.0;
    }

    double degreesOfFreedom =
        degreesNumerator / degreesDenominator;

    double betaInput =
        degreesOfFreedom
        / (
            degreesOfFreedom
            + tStatistic * tStatistic
        );

    double pValue =
        regularizedIncompleteBeta(
            betaInput,
            degreesOfFreedom / 2.0,
            0.5
        );

    return std::clamp(
        pValue,
        0.0,
        1.0
    );
}

std::vector<double>
StatisticsUtilities::benjaminiHochberg(
    const std::vector<double>& pValues
)
{
    if (pValues.empty())
    {
        return {};
    }

    std::vector<
        std::pair<double, std::size_t>
    > sortedValues;

    sortedValues.reserve(pValues.size());

    for (std::size_t index = 0;
         index < pValues.size();
         ++index)
    {
        sortedValues.push_back(
            {
                std::clamp(
                    pValues[index],
                    0.0,
                    1.0
                ),
                index
            }
        );
    }

    std::sort(
        sortedValues.begin(),
        sortedValues.end(),
        [](
            const auto& first,
            const auto& second
        )
        {
            return first.first < second.first;
        }
    );

    std::vector<double> adjustedValues(
        pValues.size(),
        1.0
    );

    double runningMinimum = 1.0;
    double testCount =
        static_cast<double>(pValues.size());

    for (std::size_t reverseIndex =
             sortedValues.size();
         reverseIndex > 0;
         --reverseIndex)
    {
        std::size_t sortedIndex =
            reverseIndex - 1;

        double rank =
            static_cast<double>(
                sortedIndex + 1
            );

        double adjusted =
            sortedValues[sortedIndex].first
            * testCount
            / rank;

        runningMinimum = std::min(
            runningMinimum,
            adjusted
        );

        adjustedValues[
            sortedValues[sortedIndex].second
        ] = std::clamp(
            runningMinimum,
            0.0,
            1.0
        );
    }

    return adjustedValues;
}

double StatisticsUtilities::
regularizedIncompleteBeta(
    double x,
    double firstShape,
    double secondShape
)
{
    if (x <= 0.0)
    {
        return 0.0;
    }

    if (x >= 1.0)
    {
        return 1.0;
    }

    double betaTerm = std::exp(
        std::lgamma(
            firstShape + secondShape
        )
        - std::lgamma(firstShape)
        - std::lgamma(secondShape)
        + firstShape * std::log(x)
        + secondShape * std::log1p(-x)
    );

    if (
        x
        < (
            firstShape + 1.0
        ) / (
            firstShape
            + secondShape
            + 2.0
        )
    )
    {
        return betaTerm
            * betaContinuedFraction(
                firstShape,
                secondShape,
                x
            )
            / firstShape;
    }

    return 1.0
        - betaTerm
        * betaContinuedFraction(
            secondShape,
            firstShape,
            1.0 - x
        )
        / secondShape;
}

double StatisticsUtilities::
betaContinuedFraction(
    double firstShape,
    double secondShape,
    double x
)
{
    constexpr int maximumIterations = 200;
    constexpr double accuracy = 3.0e-14;
    constexpr double minimumValue = 1.0e-300;

    double combinedShape =
        firstShape + secondShape;

    double firstShapePlusOne =
        firstShape + 1.0;

    double firstShapeMinusOne =
        firstShape - 1.0;

    double cValue = 1.0;

    double dValue =
        1.0
        - combinedShape * x
        / firstShapePlusOne;

    if (std::abs(dValue) < minimumValue)
    {
        dValue = minimumValue;
    }

    dValue = 1.0 / dValue;

    double result = dValue;

    for (int iteration = 1;
         iteration <= maximumIterations;
         ++iteration)
    {
        int doubledIteration =
            2 * iteration;

        double coefficient =
            iteration
            * (
                secondShape - iteration
            )
            * x
            / (
                (
                    firstShapeMinusOne
                    + doubledIteration
                )
                * (
                    firstShape
                    + doubledIteration
                )
            );

        dValue =
            1.0 + coefficient * dValue;

        if (std::abs(dValue) < minimumValue)
        {
            dValue = minimumValue;
        }

        cValue =
            1.0 + coefficient / cValue;

        if (std::abs(cValue) < minimumValue)
        {
            cValue = minimumValue;
        }

        dValue = 1.0 / dValue;
        result *= dValue * cValue;

        coefficient =
            -(
                firstShape + iteration
            )
            * (
                combinedShape + iteration
            )
            * x
            / (
                (
                    firstShape
                    + doubledIteration
                )
                * (
                    firstShapePlusOne
                    + doubledIteration
                )
            );

        dValue =
            1.0 + coefficient * dValue;

        if (std::abs(dValue) < minimumValue)
        {
            dValue = minimumValue;
        }

        cValue =
            1.0 + coefficient / cValue;

        if (std::abs(cValue) < minimumValue)
        {
            cValue = minimumValue;
        }

        dValue = 1.0 / dValue;

        double change =
            dValue * cValue;

        result *= change;

        if (std::abs(change - 1.0) < accuracy)
        {
            break;
        }
    }

    return result;
}