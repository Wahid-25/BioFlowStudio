#include "ExpressionFilter.h"

#include <algorithm>
#include <cctype>
#include <cmath>

std::string ExpressionFilterEngine::lowerCase(
    const std::string& text
) const
{
    std::string lowered = text;

    std::transform(
        lowered.begin(),
        lowered.end(),
        lowered.begin(),
        [](unsigned char character)
        {
            return static_cast<char>(std::tolower(character));
        }
    );

    return lowered;
}

bool ExpressionFilterEngine::matchesRegulation(
    RegulationStatus status,
    RegulationFilter filter
) const
{
    if (filter == RegulationFilter::All)
    {
        return true;
    }

    if (filter == RegulationFilter::Upregulated)
    {
        return status == RegulationStatus::Upregulated;
    }

    if (filter == RegulationFilter::Downregulated)
    {
        return status == RegulationStatus::Downregulated;
    }

    return status == RegulationStatus::NotSignificant;
}

std::vector<DifferentialExpressionResult>
ExpressionFilterEngine::classify(
    const std::vector<DifferentialExpressionResult>& results,
    const ExpressionFilterSettings& settings
) const
{
    std::vector<DifferentialExpressionResult> classified = results;

    for (DifferentialExpressionResult& result : classified)
    {
        RegulationStatus status = RegulationStatus::NotSignificant;

        if (result.getAdjustedPValue()
                <= settings.adjustedPValueThreshold
            && result.getLog2FoldChange()
                >= settings.minimumAbsoluteLog2FoldChange)
        {
            status = RegulationStatus::Upregulated;
        }
        else if (result.getAdjustedPValue()
                     <= settings.adjustedPValueThreshold
                 && result.getLog2FoldChange()
                     <= -settings.minimumAbsoluteLog2FoldChange)
        {
            status = RegulationStatus::Downregulated;
        }

        result.setRegulationStatus(status);
    }

    return classified;
}

std::vector<DifferentialExpressionResult>
ExpressionFilterEngine::filter(
    const std::vector<DifferentialExpressionResult>& classifiedResults,
    const ExpressionFilterSettings& settings
) const
{
    std::vector<DifferentialExpressionResult> visibleResults;
    std::string query = lowerCase(settings.geneQuery);

    for (const DifferentialExpressionResult& result : classifiedResults)
    {
        bool matchesGene = query.empty()
            || lowerCase(result.getGeneName()).find(query)
                != std::string::npos;

        if (matchesGene
            && matchesRegulation(
                result.getRegulationStatus(),
                settings.regulationFilter
            ))
        {
            visibleResults.push_back(result);
        }
    }

    std::stable_sort(
        visibleResults.begin(),
        visibleResults.end(),
        [](const DifferentialExpressionResult& left,
           const DifferentialExpressionResult& right)
        {
            return left.getAdjustedPValue()
                < right.getAdjustedPValue();
        }
    );

    if (settings.maximumResults > 0
        && visibleResults.size() > settings.maximumResults)
    {
        visibleResults.erase(
            visibleResults.begin()
                + static_cast<std::ptrdiff_t>(settings.maximumResults),
            visibleResults.end()
        );
    }

    return visibleResults;
}
