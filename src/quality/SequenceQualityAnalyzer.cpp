#include "SequenceQualityAnalyzer.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <numeric>
#include <set>
#include <sstream>

namespace
{
std::string formatNumber(double value, int precision = 2)
{
    std::ostringstream output;
    output << std::fixed << std::setprecision(precision) << value;
    return output.str();
}
}

SequenceQualityAnalyzer::SequenceQualityAnalyzer(
    const std::vector<std::unique_ptr<Sequence>>& sequences,
    const std::vector<std::string>& sourceFiles
)
    : sequences(sequences),
      sourceFiles(sourceFiles)
{
}

std::string SequenceQualityAnalyzer::getName() const
{
    return "FASTA Sequence Quality Control";
}

std::size_t SequenceQualityAnalyzer::countInvalidCharacters(
    bool nucleotideData
) const
{
    const std::string allowed = nucleotideData
        ? "ACGTUNRYKMSWBDHV-"
        : "ABCDEFGHIKLMNPQRSTVWXYZJUO*-";

    std::size_t invalidCount = 0;

    for (const std::string& filePath : sourceFiles)
    {
        std::ifstream input(filePath);
        std::string line;

        while (std::getline(input, line))
        {
            if (!line.empty() && line.front() == '>')
            {
                continue;
            }

            for (unsigned char character : line)
            {
                if (std::isspace(character))
                {
                    continue;
                }

                char upper = static_cast<char>(std::toupper(character));

                if (allowed.find(upper) == std::string::npos)
                {
                    ++invalidCount;
                }
            }
        }
    }

    return invalidCount;
}

double SequenceQualityAnalyzer::calculateGCPercentage() const
{
    std::size_t gcCount = 0;
    std::size_t canonicalBaseCount = 0;

    for (const std::string& filePath : sourceFiles)
    {
        std::ifstream input(filePath);
        std::string line;

        while (std::getline(input, line))
        {
            if (!line.empty() && line.front() == '>')
            {
                continue;
            }

            for (unsigned char character : line)
            {
                char upper = static_cast<char>(std::toupper(character));

                if (upper == 'A' || upper == 'C' || upper == 'G'
                    || upper == 'T' || upper == 'U')
                {
                    ++canonicalBaseCount;

                    if (upper == 'G' || upper == 'C')
                    {
                        ++gcCount;
                    }
                }
            }
        }
    }

    if (canonicalBaseCount == 0)
    {
        return 0.0;
    }

    return 100.0 * static_cast<double>(gcCount)
        / static_cast<double>(canonicalBaseCount);
}

QualityReport SequenceQualityAnalyzer::analyze() const
{
    QualityReport report(getName());

    report.addCheck({
        "Sequence count",
        sequences.size() >= 2 ? QualityStatus::Pass : QualityStatus::Fail,
        std::to_string(sequences.size()) + " sequence(s) loaded.",
        sequences.size() >= 2
            ? "Enough sequences are available for pairwise analysis."
            : "Import at least two sequences."
    });

    if (sequences.empty())
    {
        return report;
    }

    std::set<std::string> sequenceTypes;
    std::set<std::string> identifiers;
    bool duplicateIdentifier = false;
    std::vector<std::size_t> lengths;

    for (const auto& sequence : sequences)
    {
        sequenceTypes.insert(sequence->getTypeName());
        lengths.push_back(sequence->getLength());

        if (!identifiers.insert(sequence->getIdentifier()).second)
        {
            duplicateIdentifier = true;
        }
    }

    bool consistentType = sequenceTypes.size() == 1;

    report.addCheck({
        "Sequence-type consistency",
        consistentType ? QualityStatus::Pass : QualityStatus::Fail,
        consistentType
            ? "All sequences are " + *sequenceTypes.begin() + "."
            : "The dataset contains mixed sequence types.",
        consistentType
            ? "The sequences can be compared using one scoring model."
            : "Separate nucleotide and protein sequences before analysis."
    });

    report.addCheck({
        "Unique identifiers",
        duplicateIdentifier ? QualityStatus::Fail : QualityStatus::Pass,
        duplicateIdentifier
            ? "Duplicate FASTA identifiers were detected."
            : "All FASTA identifiers are unique.",
        duplicateIdentifier
            ? "Rename duplicate records so every sequence is identifiable."
            : "No correction is required."
    });

    auto minimumMaximum = std::minmax_element(
        lengths.begin(),
        lengths.end()
    );
    std::size_t minimumLength = *minimumMaximum.first;
    std::size_t maximumLength = *minimumMaximum.second;
    double averageLength = static_cast<double>(std::accumulate(
        lengths.begin(),
        lengths.end(),
        std::size_t{0}
    )) / static_cast<double>(lengths.size());

    report.addCheck({
        "Sequence-length summary",
        minimumLength > 0 ? QualityStatus::Pass : QualityStatus::Fail,
        "Minimum: " + std::to_string(minimumLength)
            + " | Maximum: " + std::to_string(maximumLength)
            + " | Average: " + formatNumber(averageLength),
        minimumLength > 0
            ? "Review the range before choosing a distance method."
            : "Remove empty sequences."
    });

    bool equalLengths = minimumLength == maximumLength;

    report.addCheck({
        "Equal-length requirement",
        equalLengths ? QualityStatus::Pass : QualityStatus::Warning,
        equalLengths
            ? "All sequences have equal length."
            : "Sequence lengths differ by "
                + std::to_string(maximumLength - minimumLength)
                + " position(s).",
        equalLengths
            ? "Hamming distance and Needleman-Wunsch are both available."
            : "Use Needleman-Wunsch; Hamming distance requires equal lengths."
    });

    bool nucleotideData = consistentType
        && *sequenceTypes.begin() == "Nucleotide";
    std::size_t invalidCharacters = countInvalidCharacters(nucleotideData);

    report.addCheck({
        "Valid biological characters",
        invalidCharacters == 0 ? QualityStatus::Pass : QualityStatus::Fail,
        invalidCharacters == 0
            ? "No invalid sequence characters were detected."
            : std::to_string(invalidCharacters)
                + " invalid character(s) detected.",
        invalidCharacters == 0
            ? "The FASTA content passed the character check."
            : "Correct or remove invalid characters before analysis."
    });

    if (nucleotideData)
    {
        double gcPercentage = calculateGCPercentage();
        bool extremeGC = gcPercentage < 20.0 || gcPercentage > 80.0;

        report.addCheck({
            "GC content",
            extremeGC ? QualityStatus::Warning : QualityStatus::Pass,
            "Overall GC content: " + formatNumber(gcPercentage) + "%.",
            extremeGC
                ? "Extreme GC content may indicate unusual biology or bias."
                : "GC content is within a broad expected range."
        });
    }
    else
    {
        report.addCheck({
            "GC content",
            QualityStatus::Pass,
            "Not applicable to protein sequences.",
            "No action is required."
        });
    }

    bool hammingReady = sequences.size() >= 2
        && consistentType
        && equalLengths
        && invalidCharacters == 0;

    report.addCheck({
        "Hamming-distance readiness",
        hammingReady ? QualityStatus::Pass : QualityStatus::Warning,
        hammingReady
            ? "The dataset meets Hamming-distance requirements."
            : "One or more Hamming-distance requirements are not met.",
        hammingReady
            ? "Hamming or Needleman-Wunsch can be selected."
            : "Use Needleman-Wunsch or correct the reported issues."
    });

    return report;
}
