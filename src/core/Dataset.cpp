#include "Dataset.h"

Dataset::Dataset(
    const std::string& name,
    const std::string& sourceFile
)
    : name(name),
      sourceFile(sourceFile),
      status(DatasetStatus::NotLoaded)
{
}

const std::string& Dataset::getName() const
{
    return name;
}

const std::string& Dataset::getSourceFile() const
{
    return sourceFile;
}

DatasetStatus Dataset::getStatus() const
{
    return status;
}

void Dataset::setName(const std::string& name)
{
    this->name = name;
}

void Dataset::setSourceFile(const std::string& sourceFile)
{
    this->sourceFile = sourceFile;
}

void Dataset::setStatus(DatasetStatus status)
{
    this->status = status;
}

std::string Dataset::getStatusName() const
{
    switch (status)
    {
        case DatasetStatus::Loaded:
            return "Loaded";

        case DatasetStatus::Valid:
            return "Valid";

        case DatasetStatus::Invalid:
            return "Invalid";

        default:
            return "Not Loaded";
    }
}