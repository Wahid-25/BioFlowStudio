#include "EnrichmentResult.h"

#include <sstream>

EnrichmentResult::EnrichmentResult(
    const std::string& pathwayId,
    const std::string& pathwayName,
    const std::string& category,
    std::size_t overlapCount,
    std::size_t pathwayGeneCount,
    double foldEnrichment,
    double pValue,
    const std::vector<std::string>& overlappingGenes
)
    : pathwayId(pathwayId),
      pathwayName(pathwayName),
      category(category),
      overlapCount(overlapCount),
      pathwayGeneCount(pathwayGeneCount),
      foldEnrichment(foldEnrichment),
      pValue(pValue),
      adjustedPValue(pValue),
      overlappingGenes(overlappingGenes)
{
}

const std::string& EnrichmentResult::getPathwayId() const
{
    return pathwayId;
}

const std::string& EnrichmentResult::getPathwayName() const
{
    return pathwayName;
}

const std::string& EnrichmentResult::getCategory() const
{
    return category;
}

std::size_t EnrichmentResult::getOverlapCount() const
{
    return overlapCount;
}

std::size_t EnrichmentResult::getPathwayGeneCount() const
{
    return pathwayGeneCount;
}

double EnrichmentResult::getFoldEnrichment() const
{
    return foldEnrichment;
}

double EnrichmentResult::getPValue() const
{
    return pValue;
}

double EnrichmentResult::getAdjustedPValue() const
{
    return adjustedPValue;
}

const std::vector<std::string>&
EnrichmentResult::getOverlappingGenes() const
{
    return overlappingGenes;
}

void EnrichmentResult::setAdjustedPValue(double value)
{
    adjustedPValue = value;
}

std::string EnrichmentResult::getOverlappingGeneList() const
{
    std::ostringstream stream;

    for (std::size_t index = 0; index < overlappingGenes.size(); ++index)
    {
        if (index > 0)
        {
            stream << ", ";
        }

        stream << overlappingGenes.at(index);
    }

    return stream.str();
}
