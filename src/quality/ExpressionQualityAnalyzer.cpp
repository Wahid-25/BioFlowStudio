#include "ExpressionQualityAnalyzer.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <limits>
#include <numeric>
#include <set>
#include <sstream>
#include <vector>

namespace
{
std::string formatNumber(double value, int precision = 2)
{
    std::ostringstream output;
    output << std::fixed << std::setprecision(precision) << value;
    return output.str();
}

bool containsDuplicates(const std::vector<std::string>& values)
{
    std::set<std::string> uniqueValues;

    for (const std::string& value : values)
    {
        if (!uniqueValues.insert(value).second)
        {
            return true;
        }
    }

    return false;
}
}

ExpressionQualityAnalyzer::ExpressionQualityAnalyzer(
    const ExpressionDataset& dataset,
    const SampleGrouping& grouping
)
    : dataset(dataset),
      grouping(grouping)
{
}

std::string ExpressionQualityAnalyzer::getName() const
{
    return "Gene-Expression Data Quality Control";
}

QualityReport ExpressionQualityAnalyzer::analyze() const
{
    QualityReport report(getName());

    const std::size_t geneCount = dataset.getGeneCount();
    const std::size_t sampleCount = dataset.getSampleCount();
    const auto& values = dataset.getValues();

    bool dimensionsReady = geneCount >= 2 && sampleCount >= 4;

    report.addCheck({
        "Dataset dimensions",
        dimensionsReady ? QualityStatus::Pass : QualityStatus::Fail,
        std::to_string(geneCount) + " genes and "
            + std::to_string(sampleCount) + " samples.",
        dimensionsReady
            ? "The matrix is large enough for the current analysis."
            : "Provide at least two genes and four biological samples."
    });

    bool duplicateGenes = containsDuplicates(dataset.getGeneNames());
    bool duplicateSamples = containsDuplicates(dataset.getSampleNames());

    report.addCheck({
        "Unique gene identifiers",
        duplicateGenes ? QualityStatus::Fail : QualityStatus::Pass,
        duplicateGenes
            ? "Duplicate gene identifiers were detected."
            : "All gene identifiers are unique.",
        duplicateGenes
            ? "Merge or rename duplicated gene rows."
            : "No correction is required."
    });

    report.addCheck({
        "Unique sample identifiers",
        duplicateSamples ? QualityStatus::Fail : QualityStatus::Pass,
        duplicateSamples
            ? "Duplicate sample identifiers were detected."
            : "All sample identifiers are unique.",
        duplicateSamples
            ? "Rename duplicated sample columns."
            : "No correction is required."
    });

    std::size_t missingValues = 0;
    std::size_t negativeValues = 0;
    std::size_t zeroValues = 0;
    std::size_t totalValues = 0;
    std::size_t constantGenes = 0;
    std::vector<double> libraryTotals(sampleCount, 0.0);

    for (const std::vector<double>& geneValues : values)
    {
        if (!geneValues.empty())
        {
            auto minimumMaximum = std::minmax_element(
                geneValues.begin(),
                geneValues.end()
            );

            if (std::abs(*minimumMaximum.second - *minimumMaximum.first)
                <= 1.0e-12)
            {
                ++constantGenes;
            }
        }

        for (std::size_t sample = 0;
             sample < geneValues.size();
             ++sample)
        {
            double value = geneValues.at(sample);
            ++totalValues;

            if (!std::isfinite(value))
            {
                ++missingValues;
                continue;
            }

            if (value < 0.0)
            {
                ++negativeValues;
            }

            if (std::abs(value) <= 1.0e-12)
            {
                ++zeroValues;
            }

            if (sample < libraryTotals.size())
            {
                libraryTotals.at(sample) += value;
            }
        }
    }

    report.addCheck({
        "Missing or non-finite values",
        missingValues == 0 ? QualityStatus::Pass : QualityStatus::Fail,
        std::to_string(missingValues)
            + " missing or non-finite value(s).",
        missingValues == 0
            ? "The matrix is numerically complete."
            : "Impute or remove missing values before analysis."
    });

    report.addCheck({
        "Negative expression values",
        negativeValues == 0 ? QualityStatus::Pass : QualityStatus::Fail,
        std::to_string(negativeValues) + " negative value(s).",
        negativeValues == 0
            ? "Values are compatible with the normalization strategies."
            : "Verify whether the file is already transformed or incorrect."
    });

    double zeroPercentage = totalValues == 0
        ? 0.0
        : 100.0 * static_cast<double>(zeroValues)
            / static_cast<double>(totalValues);

    report.addCheck({
        "Zero-expression proportion",
        zeroPercentage > 50.0
            ? QualityStatus::Warning
            : QualityStatus::Pass,
        formatNumber(zeroPercentage) + "% of matrix values are zero.",
        zeroPercentage > 50.0
            ? "Consider filtering very low-expression genes."
            : "The zero proportion is acceptable for this workflow."
    });

    double constantPercentage = geneCount == 0
        ? 0.0
        : 100.0 * static_cast<double>(constantGenes)
            / static_cast<double>(geneCount);

    QualityStatus constantStatus = QualityStatus::Pass;

    if (constantGenes == geneCount && geneCount > 0)
    {
        constantStatus = QualityStatus::Fail;
    }
    else if (constantGenes > 0)
    {
        constantStatus = QualityStatus::Warning;
    }

    report.addCheck({
        "Constant genes",
        constantStatus,
        std::to_string(constantGenes) + " constant gene(s), representing "
            + formatNumber(constantPercentage) + "% of genes.",
        constantGenes > 0
            ? "Constant genes can be removed because they add no variance."
            : "All genes contain measurable variation."
    });

    double minimumLibrary = libraryTotals.empty()
        ? 0.0
        : *std::min_element(libraryTotals.begin(), libraryTotals.end());
    double maximumLibrary = libraryTotals.empty()
        ? 0.0
        : *std::max_element(libraryTotals.begin(), libraryTotals.end());
    double libraryRatio = minimumLibrary > 0.0
        ? maximumLibrary / minimumLibrary
        : std::numeric_limits<double>::infinity();
    bool imbalancedLibraries = !std::isfinite(libraryRatio)
        || libraryRatio > 5.0;

    report.addCheck({
        "Sample library totals",
        imbalancedLibraries ? QualityStatus::Warning : QualityStatus::Pass,
        "Minimum total: " + formatNumber(minimumLibrary)
            + " | Maximum total: " + formatNumber(maximumLibrary)
            + " | Ratio: "
            + (std::isfinite(libraryRatio)
                ? formatNumber(libraryRatio)
                : std::string("undefined")),
        imbalancedLibraries
            ? "Inspect sample quality and use an appropriate normalization."
            : "Sample totals are within a broad comparable range."
    });

    std::size_t controlCount = grouping.getControlCount();
    std::size_t treatmentCount = grouping.getTreatmentCount();
    bool replicateReady = controlCount >= 2 && treatmentCount >= 2;

    report.addCheck({
        "Biological replicates",
        replicateReady ? QualityStatus::Pass : QualityStatus::Fail,
        std::to_string(controlCount) + " Control and "
            + std::to_string(treatmentCount) + " Treatment sample(s).",
        replicateReady
            ? "Both groups meet the minimum replicate requirement."
            : "Assign at least two samples to each group."
    });

    std::size_t groupDifference = controlCount > treatmentCount
        ? controlCount - treatmentCount
        : treatmentCount - controlCount;

    report.addCheck({
        "Group balance",
        groupDifference <= 1
            ? QualityStatus::Pass
            : QualityStatus::Warning,
        "The group-size difference is "
            + std::to_string(groupDifference) + " sample(s).",
        groupDifference <= 1
            ? "The experimental groups are reasonably balanced."
            : "Interpret results carefully or add samples to the smaller group."
    });

    return report;
}
