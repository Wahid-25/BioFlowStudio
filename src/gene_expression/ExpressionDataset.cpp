#include "ExpressionDataset.h"

#include <cmath>
#include <stdexcept>
#include <unordered_set>

ExpressionDataset::ExpressionDataset(
    const std::string& name,
    const std::string& sourceFile
)
    : Dataset(name, sourceFile)
{
}

std::string ExpressionDataset::getTypeName() const
{
    return "Gene Expression Dataset";
}

void ExpressionDataset::setData(
    const std::vector<std::string>& samples,
    const std::vector<std::string>& genes,
    const std::vector<std::vector<double>>& expressionValues
)
{
    sampleNames = samples;
    geneNames = genes;
    values = expressionValues;

    setStatus(DatasetStatus::Loaded);
}

bool ExpressionDataset::validate()
{
    if (sampleNames.size() < 2
        || geneNames.empty()
        || values.size() != geneNames.size())
    {
        setStatus(DatasetStatus::Invalid);
        return false;
    }

    std::unordered_set<std::string> uniqueSamples;
    std::unordered_set<std::string> uniqueGenes;

    for (const std::string& sample : sampleNames)
    {
        if (sample.empty()
            || !uniqueSamples.insert(sample).second)
        {
            setStatus(DatasetStatus::Invalid);
            return false;
        }
    }

    for (std::size_t row = 0;
         row < geneNames.size();
         ++row)
    {
        if (geneNames[row].empty()
            || !uniqueGenes.insert(geneNames[row]).second)
        {
            setStatus(DatasetStatus::Invalid);
            return false;
        }

        if (values[row].size() != sampleNames.size())
        {
            setStatus(DatasetStatus::Invalid);
            return false;
        }

        for (double value : values[row])
        {
            if (!std::isfinite(value)
                || value < 0.0)
            {
                setStatus(DatasetStatus::Invalid);
                return false;
            }
        }
    }

    setStatus(DatasetStatus::Valid);
    return true;
}

const std::vector<std::string>&
ExpressionDataset::getSampleNames() const
{
    return sampleNames;
}

const std::vector<std::string>&
ExpressionDataset::getGeneNames() const
{
    return geneNames;
}

const std::vector<std::vector<double>>&
ExpressionDataset::getValues() const
{
    return values;
}

std::size_t ExpressionDataset::getSampleCount() const
{
    return sampleNames.size();
}

std::size_t ExpressionDataset::getGeneCount() const
{
    return geneNames.size();
}

double ExpressionDataset::getValue(
    std::size_t geneIndex,
    std::size_t sampleIndex
) const
{
    if (geneIndex >= values.size()
        || sampleIndex >= sampleNames.size())
    {
        throw std::out_of_range(
            "Expression data index is outside its range."
        );
    }

    return values[geneIndex][sampleIndex];
}

void ExpressionDataset::clear()
{
    sampleNames.clear();
    geneNames.clear();
    values.clear();

    setStatus(DatasetStatus::NotLoaded);
}