#include "DifferentialExpressionResult.h"

DifferentialExpressionResult::
DifferentialExpressionResult(
    const std::string& geneName,
    double controlMean,
    double treatmentMean,
    double log2FoldChange,
    double pValue,
    double adjustedPValue,
    RegulationStatus regulationStatus
)
    : geneName(geneName),
      controlMean(controlMean),
      treatmentMean(treatmentMean),
      log2FoldChange(log2FoldChange),
      pValue(pValue),
      adjustedPValue(adjustedPValue),
      regulationStatus(regulationStatus)
{
}

const std::string&
DifferentialExpressionResult::getGeneName() const
{
    return geneName;
}

double DifferentialExpressionResult::
getControlMean() const
{
    return controlMean;
}

double DifferentialExpressionResult::
getTreatmentMean() const
{
    return treatmentMean;
}

double DifferentialExpressionResult::
getLog2FoldChange() const
{
    return log2FoldChange;
}

double DifferentialExpressionResult::
getPValue() const
{
    return pValue;
}

double DifferentialExpressionResult::
getAdjustedPValue() const
{
    return adjustedPValue;
}

RegulationStatus DifferentialExpressionResult::
getRegulationStatus() const
{
    return regulationStatus;
}

std::string DifferentialExpressionResult::
getRegulationName() const
{
    switch (regulationStatus)
    {
        case RegulationStatus::Upregulated:
            return "Upregulated";

        case RegulationStatus::Downregulated:
            return "Downregulated";

        default:
            return "Not Significant";
    }
}

bool DifferentialExpressionResult::
isSignificant() const
{
    return regulationStatus
        != RegulationStatus::NotSignificant;
}

void DifferentialExpressionResult::
setAdjustedPValue(double value)
{
    adjustedPValue = value;
}

void DifferentialExpressionResult::
setRegulationStatus(RegulationStatus status)
{
    regulationStatus = status;
}