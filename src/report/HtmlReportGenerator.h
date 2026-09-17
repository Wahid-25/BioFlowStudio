#pragma once

#include <string>

#include "AnalysisReport.h"

class HtmlReportGenerator
{
public:
    void generate(
        const std::string& filePath,
        const AnalysisReport& report
    ) const;

private:
    std::string escapeHtml(const std::string& text) const;
};
