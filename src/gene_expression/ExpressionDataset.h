#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "core/Dataset.h"

class ExpressionDataset : public Dataset
{
private:
    std::vector<std::string> sampleNames;
    std::vector<std::string> geneNames;
    std::vector<std::vector<double>> values;

public:
    ExpressionDataset(
        const std::string& name,
        const std::string& sourceFile
    );

    std::string getTypeName() const override;
    bool validate() override;

    void setData(
        const std::vector<std::string>& samples,
        const std::vector<std::string>& genes,
        const std::vector<std::vector<double>>& expressionValues
    );

    const std::vector<std::string>&
    getSampleNames() const;

    const std::vector<std::string>&
    getGeneNames() const;

    const std::vector<std::vector<double>>&
    getValues() const;

    std::size_t getSampleCount() const;
    std::size_t getGeneCount() const;

    double getValue(
        std::size_t geneIndex,
        std::size_t sampleIndex
    ) const;

    void clear();
};