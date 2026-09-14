#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include "AnalysisContext.h"
#include "AnalysisResult.h"
#include "AnalysisStep.h"

class Workflow
{
private:
    std::vector<std::unique_ptr<AnalysisStep>> steps;

public:
    void addStep(std::unique_ptr<AnalysisStep> step);

    std::vector<AnalysisResult> execute(
        AnalysisContext& context
    );

    std::size_t getStepCount() const;
    void clear();
};