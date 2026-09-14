#include "Workflow.h"

#include <utility>

void Workflow::addStep(
    std::unique_ptr<AnalysisStep> step
)
{
    if (step)
    {
        steps.push_back(std::move(step));
    }
}

std::vector<AnalysisResult> Workflow::execute(
    AnalysisContext& context
)
{
    std::vector<AnalysisResult> results;

    for (const auto& step : steps)
    {
        if (!step->canExecute(context))
        {
            results.emplace_back(
                step->getName(),
                AnalysisStatus::Skipped,
                "Required input data is missing."
            );

            continue;
        }

        results.push_back(step->execute(context));
    }

    return results;
}

std::size_t Workflow::getStepCount() const
{
    return steps.size();
}

void Workflow::clear()
{
    steps.clear();
}