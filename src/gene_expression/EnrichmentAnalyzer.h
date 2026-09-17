#pragma once

#include <string>
#include <vector>

#include "EnrichmentResult.h"
#include "PathwayDatabase.h"

class EnrichmentAnalyzer
{
public:
    virtual ~EnrichmentAnalyzer() = default;
    virtual std::string getName() const = 0;

    virtual std::vector<EnrichmentResult> analyze(
        const std::vector<std::string>& selectedGenes,
        const std::vector<std::string>& backgroundGenes,
        const std::vector<PathwayRecord>& pathways
    ) const = 0;
};
