#pragma once

#include <memory>
#include <string>
#include <vector>

#include "Sequence.h"

class FastaParser
{
public:
    std::vector<std::unique_ptr<Sequence>> parseFile(
        const std::string& filePath
    ) const;

private:
    std::unique_ptr<Sequence> createSequence(
        const std::string& header,
        const std::string& sequenceData
    ) const;

    std::string normalizeSequence(
        const std::string& sequenceData
    ) const;

    bool appearsToBeNucleotide(
        const std::string& sequenceData
    ) const;
};