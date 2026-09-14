#include "AnalysisContext.h"

void AnalysisContext::setFastaFiles(
    const std::vector<std::string>& files
)
{
    fastaFiles = files;
}

void AnalysisContext::setExpressionFile(
    const std::string& file
)
{
    expressionFile = file;
}

const std::vector<std::string>&
AnalysisContext::getFastaFiles() const
{
    return fastaFiles;
}

const std::string&
AnalysisContext::getExpressionFile() const
{
    return expressionFile;
}

bool AnalysisContext::hasFastaFiles() const
{
    return !fastaFiles.empty();
}

bool AnalysisContext::hasExpressionFile() const
{
    return !expressionFile.empty();
}

void AnalysisContext::clear()
{
    fastaFiles.clear();
    expressionFile.clear();
}