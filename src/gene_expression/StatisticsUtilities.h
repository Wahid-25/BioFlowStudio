#pragma once

#include <vector>

class StatisticsUtilities
{
public:
    static double mean(
        const std::vector<double>& values
    );

    static double sampleVariance(
        const std::vector<double>& values
    );

    static double welchPValue(
        const std::vector<double>& firstGroup,
        const std::vector<double>& secondGroup
    );

    static std::vector<double>
    benjaminiHochberg(
        const std::vector<double>& pValues
    );

private:
    static double regularizedIncompleteBeta(
        double x,
        double firstShape,
        double secondShape
    );

    static double betaContinuedFraction(
        double firstShape,
        double secondShape,
        double x
    );
};