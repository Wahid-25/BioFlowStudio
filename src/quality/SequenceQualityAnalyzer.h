#pragma once

#include <memory>
#include <string>
#include <vector>

#include "DataQualityAnalyzer.h"
#include "phylogenetics/Sequence.h"

class SequenceQualityAnalyzer : public DataQualityAnalyzer
{
private:
    const std::vector<std::unique_ptr<Sequence>>& sequences;
    std::vector<std::string> sourceFiles;

public:
    SequenceQualityAnalyzer(
        const std::vector<std::unique_ptr<Sequence>>& sequences,
        const std::vector<std::string>& sourceFiles
    );

    std::string getName() const override;
    QualityReport analyze() const override;

private:
    std::size_t countInvalidCharacters(
        bool nucleotideData
    ) const;

    double calculateGCPercentage() const;
};
