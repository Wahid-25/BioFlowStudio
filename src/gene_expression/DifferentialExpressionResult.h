#pragma once

#include <string>

enum class RegulationStatus
{
    Upregulated,
    Downregulated,
    NotSignificant
};

class DifferentialExpressionResult
{
private:
    std::string geneName;

    double controlMean;
    double treatmentMean;
    double log2FoldChange;
    double pValue;
    double adjustedPValue;

    RegulationStatus regulationStatus;

public:
    DifferentialExpressionResult(
        const std::string& geneName,
        double controlMean,
        double treatmentMean,
        double log2FoldChange,
        double pValue,
        double adjustedPValue,
        RegulationStatus regulationStatus
    );

    const std::string& getGeneName() const;

    double getControlMean() const;
    double getTreatmentMean() const;
    double getLog2FoldChange() const;
    double getPValue() const;
    double getAdjustedPValue() const;

    RegulationStatus getRegulationStatus() const;
    std::string getRegulationName() const;

    bool isSignificant() const;

    void setAdjustedPValue(
        double adjustedPValue
    );

    void setRegulationStatus(
        RegulationStatus status
    );
};