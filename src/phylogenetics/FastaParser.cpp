#include "FastaParser.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>

std::vector<std::unique_ptr<Sequence>>
FastaParser::parseFile(const std::string& filePath) const
{
    std::ifstream inputFile(filePath);

    if (!inputFile.is_open())
    {
        throw std::runtime_error(
            "Could not open FASTA file: " + filePath
        );
    }

    std::vector<std::unique_ptr<Sequence>> sequences;

    std::string line;
    std::string currentHeader;
    std::string currentSequence;
    std::size_t lineNumber = 0;

    auto saveCurrentSequence = [&]()
    {
        if (currentHeader.empty())
        {
            return;
        }

        if (currentSequence.empty())
        {
            throw std::runtime_error(
                "FASTA record has no sequence data: "
                + currentHeader
            );
        }

        sequences.push_back(
            createSequence(currentHeader, currentSequence)
        );

        currentSequence.clear();
    };

    while (std::getline(inputFile, line))
    {
        ++lineNumber;

        if (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }

        if (line.empty() || line.front() == ';')
        {
            continue;
        }

        if (line.front() == '>')
        {
            saveCurrentSequence();
            currentHeader = line.substr(1);

            if (currentHeader.empty())
            {
                throw std::runtime_error(
                    "Empty FASTA header at line "
                    + std::to_string(lineNumber)
                );
            }
        }
        else
        {
            if (currentHeader.empty())
            {
                throw std::runtime_error(
                    "Sequence data found before a FASTA header."
                );
            }

            currentSequence += line;
        }
    }

    saveCurrentSequence();

    if (sequences.empty())
    {
        throw std::runtime_error(
            "The selected file contains no FASTA records."
        );
    }

    return sequences;
}

std::unique_ptr<Sequence> FastaParser::createSequence(
    const std::string& header,
    const std::string& sequenceData
) const
{
    std::istringstream headerStream(header);

    std::string identifier;
    headerStream >> identifier;

    std::string description;
    std::getline(headerStream, description);

    if (!description.empty() && description.front() == ' ')
    {
        description.erase(0, 1);
    }

    std::string normalizedData =
        normalizeSequence(sequenceData);

    std::unique_ptr<Sequence> sequence;

    if (appearsToBeNucleotide(normalizedData))
    {
        sequence = std::make_unique<NucleotideSequence>(
            identifier,
            description,
            normalizedData
        );
    }
    else
    {
        sequence = std::make_unique<ProteinSequence>(
            identifier,
            description,
            normalizedData
        );
    }

    if (!sequence->validate())
    {
        throw std::runtime_error(
            "Invalid biological sequence: " + identifier
        );
    }

    return sequence;
}

std::string FastaParser::normalizeSequence(
    const std::string& sequenceData
) const
{
    std::string normalizedData;

    for (char character : sequenceData)
    {
        if (!std::isspace(
                static_cast<unsigned char>(character)
            ))
        {
            normalizedData += static_cast<char>(
                std::toupper(
                    static_cast<unsigned char>(character)
                )
            );
        }
    }

    return normalizedData;
}

bool FastaParser::appearsToBeNucleotide(
    const std::string& sequenceData
) const
{
    const std::string nucleotideCharacters =
        "ACGTUNRYSWKMBDHV-";

    return !sequenceData.empty()
        && std::all_of(
            sequenceData.begin(),
            sequenceData.end(),
            [&nucleotideCharacters](char character)
            {
                return nucleotideCharacters.find(character)
                       != std::string::npos;
            }
        );
}