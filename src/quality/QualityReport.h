#pragma once

#include <cstddef>
#include <string>
#include <vector>

enum class QualityStatus
{
    Pass,
    Warning,
    Fail
};

struct QualityCheck
{
    std::string name;
    QualityStatus status;
    std::string observation;
    std::string recommendation;
};

class QualityReport
{
private:
    std::string title;
    std::vector<QualityCheck> checks;

public:
    explicit QualityReport(const std::string& title);

    void addCheck(const QualityCheck& check);

    const std::string& getTitle() const;
    const std::vector<QualityCheck>& getChecks() const;

    QualityStatus getOverallStatus() const;
    std::string getOverallStatusName() const;

    std::size_t getPassCount() const;
    std::size_t getWarningCount() const;
    std::size_t getFailCount() const;

    static std::string statusName(QualityStatus status);
};
