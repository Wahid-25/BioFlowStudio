#include "PathwayDatabase.h"

std::string BuiltInPathwayDatabase::getName() const
{
    return "BioFlow Curated Teaching Pathways";
}

std::vector<PathwayRecord> BuiltInPathwayDatabase::getPathways() const
{
    return {
        {
            "BFS-PW001", "p53 stress-response signalling",
            "Signalling Pathway",
            {"TP53", "MDM2", "CDKN1A", "BAX", "BCL2", "CASP3",
             "PTEN", "ATM", "CHEK2"}
        },
        {
            "BFS-PW002", "PI3K-AKT growth signalling",
            "Signalling Pathway",
            {"AKT1", "MTOR", "PTEN", "EGFR", "VEGFA", "MYC",
             "BCL2", "CCND1"}
        },
        {
            "BFS-PW003", "Cell-cycle progression",
            "GO Biological Process",
            {"CDK1", "MKI67", "CCND1", "MYC", "TP53", "MDM2",
             "RB1", "E2F1"}
        },
        {
            "BFS-PW004", "Programmed cell death",
            "GO Biological Process",
            {"TP53", "BAX", "BCL2", "CASP3", "CASP8", "TNF",
             "FAS", "AKT1"}
        },
        {
            "BFS-PW005", "Inflammatory response",
            "GO Biological Process",
            {"IL6", "TNF", "CXCL8", "STAT3", "NFKB1", "RELA",
             "JUN", "FOS"}
        },
        {
            "BFS-PW006", "Hypoxia and HIF response",
            "Stress Response",
            {"HIF1A", "VEGFA", "MTOR", "AKT1", "EGFR", "MYC"}
        },
        {
            "BFS-PW007", "MAPK mitogenic signalling",
            "Signalling Pathway",
            {"MAPK1", "EGFR", "MYC", "JUN", "FOS", "TNF",
             "CCND1"}
        },
        {
            "BFS-PW008", "DNA-damage repair",
            "GO Biological Process",
            {"BRCA1", "BRCA2", "ATM", "CHEK2", "TP53", "RAD51",
             "MDM2"}
        },
        {
            "BFS-PW009", "Angiogenesis",
            "GO Biological Process",
            {"VEGFA", "HIF1A", "EGFR", "IL6", "CXCL8", "AKT1"}
        },
        {
            "BFS-PW010", "mTOR nutrient signalling",
            "Signalling Pathway",
            {"MTOR", "AKT1", "PTEN", "MYC", "HIF1A"}
        },
        {
            "BFS-PW011", "JAK-STAT cytokine signalling",
            "Signalling Pathway",
            {"IL6", "STAT3", "MYC", "BCL2", "JAK1", "JAK2"}
        },
        {
            "BFS-PW012", "TNF inflammatory signalling",
            "Signalling Pathway",
            {"TNF", "CXCL8", "IL6", "NFKB1", "RELA", "CASP3",
             "JUN"}
        },
        {
            "BFS-PW013", "Positive regulation of proliferation",
            "GO Biological Process",
            {"MYC", "CDK1", "MKI67", "CCND1", "EGFR", "AKT1",
             "STAT3"}
        },
        {
            "BFS-PW014", "Tumour-suppressor network",
            "Disease Process",
            {"TP53", "PTEN", "BRCA1", "BRCA2", "ATM", "CHEK2",
             "RB1"}
        },
        {
            "BFS-PW015", "Cancer survival and invasion",
            "Disease Process",
            {"MYC", "VEGFA", "EGFR", "AKT1", "MTOR", "BCL2",
             "STAT3", "CXCL8", "MKI67"}
        }
    };
}
