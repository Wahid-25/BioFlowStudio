#pragma once

#include <string>

#include "AnalysisContext.h"
#include "AnalysisResult.h"

class AnalysisStep
{
public:
    virtual ~AnalysisStep() = default;

    virtual std::string getName() const = 0;

    virtual bool canExecute(
        const AnalysisContext& context
    ) const = 0;

    virtual AnalysisResult execute(
        AnalysisContext& context
    ) = 0;
};