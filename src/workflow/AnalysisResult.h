#pragma once

#include <string>
#include <vector>

enum class AnalysisStatus
{
    NotStarted,
    Success,
    Failure,
    Skipped
};

class AnalysisResult
{
private:
    std::string stepName;
    AnalysisStatus status;
    std::string message;
    std::vector<std::string> warnings;
    std::vector<std::string> outputFiles;

public:
    AnalysisResult(
        const std::string& stepName,
        AnalysisStatus status,
        const std::string& message
    );

    const std::string& getStepName() const;
    AnalysisStatus getStatus() const;
    const std::string& getMessage() const;

    void addWarning(const std::string& warning);
    void addOutputFile(const std::string& file);

    const std::vector<std::string>& getWarnings() const;
    const std::vector<std::string>& getOutputFiles() const;

    bool isSuccessful() const;
};