#include "HypergeometricEnrichmentAnalyzer.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

std::string HypergeometricEnrichmentAnalyzer::getName() const
{
    return "Hypergeometric Overrepresentation Analysis";
}

std::string HypergeometricEnrichmentAnalyzer::upperCase(
    const std::string& text
) const
{
    std::string normalized = text;

    std::transform(
        normalized.begin(), normalized.end(), normalized.begin(),
        [](unsigned char character)
        {
            return static_cast<char>(std::toupper(character));
        }
    );

    return normalized;
}

double HypergeometricEnrichmentAnalyzer::logCombination(
    std::size_t n,
    std::size_t k
) const
{
    if (k > n)
    {
        return -std::numeric_limits<double>::infinity();
    }

    return std::lgamma(static_cast<double>(n) + 1.0)
        - std::lgamma(static_cast<double>(k) + 1.0)
        - std::lgamma(static_cast<double>(n - k) + 1.0);
}

double HypergeometricEnrichmentAnalyzer::rightTailPValue(
    std::size_t populationSize,
    std::size_t pathwayGenes,
    std::size_t selectedGenes,
    std::size_t overlap
) const
{
    std::size_t upper = std::min(pathwayGenes, selectedGenes);
    double probability = 0.0;

    for (std::size_t value = overlap; value <= upper; ++value)
    {
        if (selectedGenes < value
            || populationSize < pathwayGenes
            || populationSize - pathwayGenes < selectedGenes - value)
        {
            continue;
        }

        double logProbability =
            logCombination(pathwayGenes, value)
            + logCombination(
                populationSize - pathwayGenes,
                selectedGenes - value
            )
            - logCombination(populationSize, selectedGenes);

        probability += std::exp(logProbability);
    }

    return std::min(1.0, probability);
}

std::vector<EnrichmentResult>
HypergeometricEnrichmentAnalyzer::analyze(
    const std::vector<std::string>& selectedGenes,
    const std::vector<std::string>& backgroundGenes,
    const std::vector<PathwayRecord>& pathways
) const
{
    if (backgroundGenes.empty())
    {
        throw std::invalid_argument(
            "Enrichment analysis requires a non-empty background gene list."
        );
    }

    std::unordered_map<std::string, std::string> backgroundNames;

    for (const std::string& gene : backgroundGenes)
    {
        backgroundNames[upperCase(gene)] = gene;
    }

    std::unordered_set<std::string> selectedSet;

    for (const std::string& gene : selectedGenes)
    {
        std::string normalized = upperCase(gene);

        if (backgroundNames.find(normalized) != backgroundNames.end())
        {
            selectedSet.insert(normalized);
        }
    }

    if (selectedSet.empty())
    {
        throw std::invalid_argument(
            "No significant genes were available for enrichment analysis."
        );
    }

    std::vector<EnrichmentResult> results;

    for (const PathwayRecord& pathway : pathways)
    {
        std::unordered_set<std::string> pathwayInBackground;

        for (const std::string& gene : pathway.genes)
        {
            std::string normalized = upperCase(gene);

            if (backgroundNames.find(normalized) != backgroundNames.end())
            {
                pathwayInBackground.insert(normalized);
            }
        }

        if (pathwayInBackground.empty())
        {
            continue;
        }

        std::vector<std::string> overlapGenes;

        for (const std::string& gene : selectedSet)
        {
            if (pathwayInBackground.find(gene)
                != pathwayInBackground.end())
            {
                overlapGenes.push_back(backgroundNames.at(gene));
            }
        }

        if (overlapGenes.empty())
        {
            continue;
        }

        std::sort(overlapGenes.begin(), overlapGenes.end());

        double observedFraction =
            static_cast<double>(overlapGenes.size())
            / static_cast<double>(selectedSet.size());
        double backgroundFraction =
            static_cast<double>(pathwayInBackground.size())
            / static_cast<double>(backgroundNames.size());
        double foldEnrichment = observedFraction / backgroundFraction;

        double pValue = rightTailPValue(
            backgroundNames.size(),
            pathwayInBackground.size(),
            selectedSet.size(),
            overlapGenes.size()
        );

        results.emplace_back(
            pathway.id,
            pathway.name,
            pathway.category,
            overlapGenes.size(),
            pathwayInBackground.size(),
            foldEnrichment,
            pValue,
            overlapGenes
        );
    }

    adjustPValues(results);

    std::stable_sort(
        results.begin(), results.end(),
        [](const EnrichmentResult& left,
           const EnrichmentResult& right)
        {
            return left.getAdjustedPValue()
                < right.getAdjustedPValue();
        }
    );

    return results;
}

void HypergeometricEnrichmentAnalyzer::adjustPValues(
    std::vector<EnrichmentResult>& results
) const
{
    if (results.empty())
    {
        return;
    }

    std::vector<std::size_t> order(results.size());

    for (std::size_t index = 0; index < order.size(); ++index)
    {
        order.at(index) = index;
    }

    std::sort(
        order.begin(), order.end(),
        [&results](std::size_t left, std::size_t right)
        {
            return results.at(left).getPValue()
                < results.at(right).getPValue();
        }
    );

    double nextAdjusted = 1.0;

    for (std::size_t rank = order.size(); rank > 0; --rank)
    {
        std::size_t resultIndex = order.at(rank - 1);
        double adjusted = results.at(resultIndex).getPValue()
            * static_cast<double>(order.size())
            / static_cast<double>(rank);

        adjusted = std::min({1.0, adjusted, nextAdjusted});
        results.at(resultIndex).setAdjustedPValue(adjusted);
        nextAdjusted = adjusted;
    }
}
