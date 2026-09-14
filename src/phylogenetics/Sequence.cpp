#include "Sequence.h"

#include <algorithm>
#include <cctype>

Sequence::Sequence(
    const std::string& identifier,
    const std::string& description,
    const std::string& sequenceData
)
    : identifier(identifier),
      description(description),
      sequenceData(sequenceData)
{
}

const std::string& Sequence::getIdentifier() const
{
    return identifier;
}

const std::string& Sequence::getDescription() const
{
    return description;
}

const std::string& Sequence::getSequenceData() const
{
    return sequenceData;
}

std::size_t Sequence::getLength() const
{
    return sequenceData.length();
}

SequenceType NucleotideSequence::getType() const
{
    return SequenceType::Nucleotide;
}

std::string NucleotideSequence::getTypeName() const
{
    return "Nucleotide";
}

bool NucleotideSequence::validate() const
{
    const std::string& data = getSequenceData();

    if (data.empty())
    {
        return false;
    }

    const std::string allowedCharacters =
        "ACGTUNRYSWKMBDHV-";

    return std::all_of(
        data.begin(),
        data.end(),
        [&allowedCharacters](char character)
        {
            char upperCharacter = static_cast<char>(
                std::toupper(
                    static_cast<unsigned char>(character)
                )
            );

            return allowedCharacters.find(upperCharacter)
                   != std::string::npos;
        }
    );
}

SequenceType ProteinSequence::getType() const
{
    return SequenceType::Protein;
}

std::string ProteinSequence::getTypeName() const
{
    return "Protein";
}

bool ProteinSequence::validate() const
{
    const std::string& data = getSequenceData();

    if (data.empty())
    {
        return false;
    }

    const std::string allowedCharacters =
        "ACDEFGHIKLMNPQRSTVWYBXZJUO*-";

    return std::all_of(
        data.begin(),
        data.end(),
        [&allowedCharacters](char character)
        {
            char upperCharacter = static_cast<char>(
                std::toupper(
                    static_cast<unsigned char>(character)
                )
            );

            return allowedCharacters.find(upperCharacter)
                   != std::string::npos;
        }
    );
}