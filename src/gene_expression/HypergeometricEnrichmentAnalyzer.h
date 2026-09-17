#pragma once

#include "EnrichmentAnalyzer.h"

class HypergeometricEnrichmentAnalyzer : public EnrichmentAnalyzer
{
public:
    std::string getName() const override;

    std::vector<EnrichmentResult> analyze(
        const std::vector<std::string>& selectedGenes,
        const std::vector<std::string>& backgroundGenes,
        const std::vector<PathwayRecord>& pathways
    ) const override;

private:
    double logCombination(std::size_t n, std::size_t k) const;

    double rightTailPValue(
        std::size_t populationSize,
        std::size_t pathwayGenes,
        std::size_t selectedGenes,
        std::size_t overlap
    ) const;

    std::string upperCase(const std::string& text) const;
    void adjustPValues(std::vector<EnrichmentResult>& results) const;
};
