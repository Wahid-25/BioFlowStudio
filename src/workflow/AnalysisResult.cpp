#include "AnalysisResult.h"

AnalysisResult::AnalysisResult(
    const std::string& stepName,
    AnalysisStatus status,
    const std::string& message
)
    : stepName(stepName),
      status(status),
      message(message)
{
}

const std::string& AnalysisResult::getStepName() const
{
    return stepName;
}

AnalysisStatus AnalysisResult::getStatus() const
{
    return status;
}

const std::string& AnalysisResult::getMessage() const
{
    return message;
}

void AnalysisResult::addWarning(const std::string& warning)
{
    warnings.push_back(warning);
}

void AnalysisResult::addOutputFile(const std::string& file)
{
    outputFiles.push_back(file);
}

const std::vector<std::string>&
AnalysisResult::getWarnings() const
{
    return warnings;
}

const std::vector<std::string>&
AnalysisResult::getOutputFiles() const
{
    return outputFiles;
}

bool AnalysisResult::isSuccessful() const
{
    return status == AnalysisStatus::Success;
}