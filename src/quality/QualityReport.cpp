#include "QualityReport.h"

#include <algorithm>

QualityReport::QualityReport(const std::string& title)
    : title(title)
{
}

void QualityReport::addCheck(const QualityCheck& check)
{
    checks.push_back(check);
}

const std::string& QualityReport::getTitle() const
{
    return title;
}

const std::vector<QualityCheck>& QualityReport::getChecks() const
{
    return checks;
}

QualityStatus QualityReport::getOverallStatus() const
{
    if (std::any_of(
            checks.begin(),
            checks.end(),
            [](const QualityCheck& check)
            {
                return check.status == QualityStatus::Fail;
            }))
    {
        return QualityStatus::Fail;
    }

    if (std::any_of(
            checks.begin(),
            checks.end(),
            [](const QualityCheck& check)
            {
                return check.status == QualityStatus::Warning;
            }))
    {
        return QualityStatus::Warning;
    }

    return QualityStatus::Pass;
}

std::string QualityReport::getOverallStatusName() const
{
    return statusName(getOverallStatus());
}

std::size_t QualityReport::getPassCount() const
{
    return static_cast<std::size_t>(std::count_if(
        checks.begin(),
        checks.end(),
        [](const QualityCheck& check)
        {
            return check.status == QualityStatus::Pass;
        }
    ));
}

std::size_t QualityReport::getWarningCount() const
{
    return static_cast<std::size_t>(std::count_if(
        checks.begin(),
        checks.end(),
        [](const QualityCheck& check)
        {
            return check.status == QualityStatus::Warning;
        }
    ));
}

std::size_t QualityReport::getFailCount() const
{
    return static_cast<std::size_t>(std::count_if(
        checks.begin(),
        checks.end(),
        [](const QualityCheck& check)
        {
            return check.status == QualityStatus::Fail;
        }
    ));
}

std::string QualityReport::statusName(QualityStatus status)
{
    switch (status)
    {
        case QualityStatus::Pass:
            return "Pass";

        case QualityStatus::Warning:
            return "Warning";

        case QualityStatus::Fail:
            return "Fail";
    }

    return "Unknown";
}
