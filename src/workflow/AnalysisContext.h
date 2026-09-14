#pragma once

#include <string>
#include <vector>

class AnalysisContext
{
private:
    std::vector<std::string> fastaFiles;
    std::string expressionFile;

public:
    void setFastaFiles(const std::vector<std::string>& files);
    void setExpressionFile(const std::string& file);

    const std::vector<std::string>& getFastaFiles() const;
    const std::string& getExpressionFile() const;

    bool hasFastaFiles() const;
    bool hasExpressionFile() const;

    void clear();
};