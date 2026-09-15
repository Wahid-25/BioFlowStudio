#pragma once

#include <string>
#include <vector>

#include "ExpressionDataset.h"

class ExpressionParser
{
public:
    ExpressionDataset parseFile(
        const std::string& filePath
    ) const;

private:
    char detectDelimiter(
        const std::string& headerLine
    ) const;

    std::vector<std::string> splitLine(
        const std::string& line,
        char delimiter
    ) const;

    std::string trim(
        const std::string& text
    ) const;
};