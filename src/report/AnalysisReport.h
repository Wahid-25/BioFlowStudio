#pragma once

#include <string>
#include <utility>
#include <vector>

struct ReportTable
{
    std::string title;
    std::vector<std::string> headers;
    std::vector<std::vector<std::string>> rows;
};

struct ReportImage
{
    std::string title;
    std::string relativeFilePath;
    std::string description;
};

struct AnalysisReport
{
    std::string title;
    std::string subtitle;
    std::string generatedAt;
    std::string overview;
    std::vector<std::pair<std::string, std::string>> metadata;
    std::vector<ReportTable> tables;
    std::vector<ReportImage> images;
    std::vector<std::string> notes;
};
