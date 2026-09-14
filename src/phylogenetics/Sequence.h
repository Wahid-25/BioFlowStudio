#pragma once

#include <string>

enum class SequenceType
{
    Nucleotide,
    Protein
};

class Sequence
{
private:
    std::string identifier;
    std::string description;
    std::string sequenceData;

public:
    Sequence(
        const std::string& identifier,
        const std::string& description,
        const std::string& sequenceData
    );

    virtual ~Sequence() = default;

    const std::string& getIdentifier() const;
    const std::string& getDescription() const;
    const std::string& getSequenceData() const;

    std::size_t getLength() const;

    virtual SequenceType getType() const = 0;
    virtual std::string getTypeName() const = 0;
    virtual bool validate() const = 0;
};

class NucleotideSequence : public Sequence
{
public:
    using Sequence::Sequence;

    SequenceType getType() const override;
    std::string getTypeName() const override;
    bool validate() const override;
};

class ProteinSequence : public Sequence
{
public:
    using Sequence::Sequence;

    SequenceType getType() const override;
    std::string getTypeName() const override;
    bool validate() const override;
};