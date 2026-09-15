#include "WelchTTestAnalyzer.h"

#include <cmath>
#include <stdexcept>
#include <vector>

#include "StatisticsUtilities.h"

std::string WelchTTestAnalyzer::getName() const
{
    return "Welch's Two-Sample T-Test";
}

std::vector<DifferentialExpressionResult>
WelchTTestAnalyzer::analyze(
    const ExpressionDataset& dataset,
    const std::vector<std::vector<double>>&
        normalizedValues,
    const SampleGrouping& grouping
) const
{
    if (!grouping.isValid())
    {
        throw std::invalid_argument(
            "At least two Control and two Treatment "
            "samples are required."
        );
    }

    if (
        grouping.getGroups().size()
        != dataset.getSampleCount()
    )
    {
        throw std::invalid_argument(
            "Sample grouping does not match "
            "the expression dataset."
        );
    }

    if (
        normalizedValues.size()
        != dataset.getGeneCount()
    )
    {
        throw std::invalid_argument(
            "Normalized matrix gene count "
            "does not match the dataset."
        );
    }

    const auto& controlIndices =
        grouping.getControlIndices();

    const auto& treatmentIndices =
        grouping.getTreatmentIndices();

    const auto& rawValues =
        dataset.getValues();

    std::vector<DifferentialExpressionResult>
        results;

    std::vector<double> pValues;

    results.reserve(dataset.getGeneCount());
    pValues.reserve(dataset.getGeneCount());

    for (std::size_t gene = 0;
         gene < dataset.getGeneCount();
         ++gene)
    {
        if (
            normalizedValues[gene].size()
            != dataset.getSampleCount()
        )
        {
            throw std::invalid_argument(
                "A normalized expression row "
                "has an incorrect sample count."
            );
        }

        std::vector<double> normalizedControl =
            extractValues(
                normalizedValues[gene],
                controlIndices
            );

        std::vector<double> normalizedTreatment =
            extractValues(
                normalizedValues[gene],
                treatmentIndices
            );

        std::vector<double> rawControl =
            extractValues(
                rawValues[gene],
                controlIndices
            );

        std::vector<double> rawTreatment =
            extractValues(
                rawValues[gene],
                treatmentIndices
            );

        double controlMean =
            StatisticsUtilities::mean(
                normalizedControl
            );

        double treatmentMean =
            StatisticsUtilities::mean(
                normalizedTreatment
            );

        double rawControlMean =
            StatisticsUtilities::mean(
                rawControl
            );

        double rawTreatmentMean =
            StatisticsUtilities::mean(
                rawTreatment
            );

        double log2FoldChange =
            std::log2(
                (
                    rawTreatmentMean + 1.0
                )
                / (
                    rawControlMean + 1.0
                )
            );

        double pValue =
            StatisticsUtilities::welchPValue(
                normalizedControl,
                normalizedTreatment
            );

        pValues.push_back(pValue);

        results.emplace_back(
            dataset.getGeneNames().at(gene),
            controlMean,
            treatmentMean,
            log2FoldChange,
            pValue,
            1.0,
            RegulationStatus::NotSignificant
        );
    }

    std::vector<double> adjustedPValues =
        StatisticsUtilities::benjaminiHochberg(
            pValues
        );

    for (std::size_t gene = 0;
         gene < results.size();
         ++gene)
    {
        results[gene].setAdjustedPValue(
            adjustedPValues[gene]
        );

        if (
            adjustedPValues[gene] < 0.05
            && results[gene].getLog2FoldChange()
                >= 1.0
        )
        {
            results[gene].setRegulationStatus(
                RegulationStatus::Upregulated
            );
        }
        else if (
            adjustedPValues[gene] < 0.05
            && results[gene].getLog2FoldChange()
                <= -1.0
        )
        {
            results[gene].setRegulationStatus(
                RegulationStatus::Downregulated
            );
        }
        else
        {
            results[gene].setRegulationStatus(
                RegulationStatus::NotSignificant
            );
        }
    }

    return results;
}

std::vector<double>
WelchTTestAnalyzer::extractValues(
    const std::vector<double>& row,
    const std::vector<std::size_t>& indices
) const
{
    std::vector<double> selectedValues;

    selectedValues.reserve(indices.size());

    for (std::size_t index : indices)
    {
        if (index >= row.size())
        {
            throw std::out_of_range(
                "Sample index is outside "
                "the expression row."
            );
        }

        selectedValues.push_back(row[index]);
    }

    return selectedValues;
}