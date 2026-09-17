#pragma once

#include <cstddef>
#include <string>
#include <vector>

class EnrichmentResult
{
private:
    std::string pathwayId;
    std::string pathwayName;
    std::string category;
    std::size_t overlapCount;
    std::size_t pathwayGeneCount;
    double foldEnrichment;
    double pValue;
    double adjustedPValue;
    std::vector<std::string> overlappingGenes;

public:
    EnrichmentResult(
        const std::string& pathwayId,
        const std::string& pathwayName,
        const std::string& category,
        std::size_t overlapCount,
        std::size_t pathwayGeneCount,
        double foldEnrichment,
        double pValue,
        const std::vector<std::string>& overlappingGenes
    );

    const std::string& getPathwayId() const;
    const std::string& getPathwayName() const;
    const std::string& getCategory() const;
    std::size_t getOverlapCount() const;
    std::size_t getPathwayGeneCount() const;
    double getFoldEnrichment() const;
    double getPValue() const;
    double getAdjustedPValue() const;
    const std::vector<std::string>& getOverlappingGenes() const;

    void setAdjustedPValue(double value);
    std::string getOverlappingGeneList() const;
};
