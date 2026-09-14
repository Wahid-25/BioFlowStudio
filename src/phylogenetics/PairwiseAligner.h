#pragma once

#include <string>

#include "Sequence.h"

class PairwiseAligner
{
public:
    virtual ~PairwiseAligner() = default;

    virtual std::string getName() const = 0;

    virtual double calculateDistance(
        const Sequence& first,
        const Sequence& second
    ) const = 0;
};

class HammingAligner : public PairwiseAligner
{
public:
    std::string getName() const override;

    double calculateDistance(
        const Sequence& first,
        const Sequence& second
    ) const override;
};

class NeedlemanWunschAligner : public PairwiseAligner
{
private:
    int matchCost;
    int mismatchCost;
    int gapCost;

public:
    NeedlemanWunschAligner(
        int matchCost = 0,
        int mismatchCost = 1,
        int gapCost = 1
    );

    std::string getName() const override;

    double calculateDistance(
        const Sequence& first,
        const Sequence& second
    ) const override;
};