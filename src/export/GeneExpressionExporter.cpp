#include "GeneExpressionExporter.h"

#include <fstream>
#include <iomanip>
#include <stdexcept>

std::string GeneExpressionExporter::escapeCSV(
    const std::string& value
) const
{
    bool requiresQuotes =
        value.find(',') != std::string::npos
        || value.find('"') != std::string::npos
        || value.find('\n') != std::string::npos
        || value.find('\r') != std::string::npos;

    if (!requiresQuotes)
    {
        return value;
    }

    std::string escaped = "\"";

    for (char character : value)
    {
        if (character == '"')
        {
            escaped += "\"\"";
        }
        else
        {
            escaped += character;
        }
    }

    escaped += "\"";
    return escaped;
}

std::string GeneExpressionExporter::sampleGroupName(
    SampleGroup group
) const
{
    if (group == SampleGroup::Control)
    {
        return "Control";
    }

    if (group == SampleGroup::Treatment)
    {
        return "Treatment";
    }

    return "Unassigned";
}

void GeneExpressionExporter::exportResultsCSV(
    const std::string& filePath,
    const std::vector<DifferentialExpressionResult>& results
) const
{
    std::ofstream output(filePath);

    if (!output)
    {
        throw std::runtime_error(
            "Could not create differential expression CSV file."
        );
    }

    output << "Gene,Control Mean,Treatment Mean,Log2 Fold Change,"
              "P-value,Adjusted P-value,Regulation\n";
    output << std::setprecision(12);

    for (const DifferentialExpressionResult& result : results)
    {
        output
            << escapeCSV(result.getGeneName()) << ','
            << result.getControlMean() << ','
            << result.getTreatmentMean() << ','
            << result.getLog2FoldChange() << ','
            << result.getPValue() << ','
            << result.getAdjustedPValue() << ','
            << escapeCSV(result.getRegulationName()) << '\n';
    }

    if (!output)
    {
        throw std::runtime_error(
            "Failed while writing differential expression CSV file."
        );
    }
}

void GeneExpressionExporter::exportNormalizedMatrixCSV(
    const std::string& filePath,
    const ExpressionDataset& dataset,
    const std::vector<std::vector<double>>& normalizedValues,
    const SampleGrouping& grouping
) const
{
    if (normalizedValues.size() != dataset.getGeneCount())
    {
        throw std::invalid_argument(
            "Normalized matrix gene count is inconsistent."
        );
    }

    std::ofstream output(filePath);

    if (!output)
    {
        throw std::runtime_error(
            "Could not create normalized expression CSV file."
        );
    }

    output << "Gene";

    const auto& sampleNames = dataset.getSampleNames();

    for (std::size_t sample = 0;
         sample < sampleNames.size();
         ++sample)
    {
        output << ',' << escapeCSV(
            sampleNames.at(sample)
            + " ["
            + sampleGroupName(grouping.getGroup(sample))
            + "]"
        );
    }

    output << '\n' << std::setprecision(12);

    const auto& geneNames = dataset.getGeneNames();

    for (std::size_t gene = 0; gene < geneNames.size(); ++gene)
    {
        if (normalizedValues.at(gene).size() != sampleNames.size())
        {
            throw std::invalid_argument(
                "Normalized matrix sample count is inconsistent."
            );
        }

        output << escapeCSV(geneNames.at(gene));

        for (double value : normalizedValues.at(gene))
        {
            output << ',' << value;
        }

        output << '\n';
    }

    if (!output)
    {
        throw std::runtime_error(
            "Failed while writing normalized expression CSV file."
        );
    }
}

void GeneExpressionExporter::exportAnalysisSummary(
    const std::string& filePath,
    const ExpressionDataset& dataset,
    const std::vector<DifferentialExpressionResult>& results,
    const SampleGrouping& grouping,
    const std::string& normalizationName
) const
{
    std::ofstream output(filePath);

    if (!output)
    {
        throw std::runtime_error(
            "Could not create analysis summary file."
        );
    }

    std::size_t upregulated = 0;
    std::size_t downregulated = 0;

    for (const DifferentialExpressionResult& result : results)
    {
        if (result.getRegulationStatus() == RegulationStatus::Upregulated)
        {
            ++upregulated;
        }
        else if (result.getRegulationStatus()
                 == RegulationStatus::Downregulated)
        {
            ++downregulated;
        }
    }

    output
        << "BioFlow Studio - Gene Expression Analysis Summary\n"
        << "=================================================\n\n"
        << "Dataset: " << dataset.getName() << '\n'
        << "Source file: " << dataset.getSourceFile() << '\n'
        << "Genes analyzed: " << dataset.getGeneCount() << '\n'
        << "Samples analyzed: " << dataset.getSampleCount() << '\n'
        << "Control samples: " << grouping.getControlCount() << '\n'
        << "Treatment samples: " << grouping.getTreatmentCount() << '\n'
        << "Normalization: " << normalizationName << '\n'
        << "Statistical test: Welch independent t-test\n"
        << "Multiple-testing correction: Benjamini-Hochberg FDR\n"
        << "Significance thresholds: adjusted p-value < 0.05 and "
           "absolute log2 fold change >= 1\n\n"
        << "Upregulated genes: " << upregulated << '\n'
        << "Downregulated genes: " << downregulated << '\n'
        << "Total significant genes: "
        << upregulated + downregulated << '\n'
        << "Not significant: "
        << results.size() - upregulated - downregulated << '\n';

    if (!output)
    {
        throw std::runtime_error(
            "Failed while writing analysis summary file."
        );
    }
}
