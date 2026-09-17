#pragma once

#include <string>

#include "QualityReport.h"

class DataQualityAnalyzer
{
public:
    virtual ~DataQualityAnalyzer() = default;

    virtual std::string getName() const = 0;
    virtual QualityReport analyze() const = 0;
};
