#pragma once

#include <QMainWindow>
#include <QString>
#include <QStringList>

#include <memory>
#include <vector>

#include "core/Project.h"
#include "gene_expression/DifferentialExpressionResult.h"
#include "gene_expression/ExpressionDataset.h"
#include "gene_expression/ExpressionFilter.h"
#include "gene_expression/SampleGrouping.h"
#include "phylogenetics/DistanceMatrix.h"
#include "phylogenetics/Sequence.h"
#include "phylogenetics/UPGMATree.h"

class QLabel;
class QListWidget;
class QComboBox;
class QDoubleSpinBox;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QStackedWidget;
class QTableWidget;
class QWidget;

class PairwiseAligner;
class ExpressionHeatmapWidget;
class PCAPlotWidget;
class PhylogeneticTreeWidget;
class VolcanoPlotWidget;

class MainWindow : public QMainWindow
{
public:
    explicit MainWindow(QWidget* parent = nullptr);

private:
    static constexpr int DashboardPage = 0;
    static constexpr int PhylogeneticSetupPage = 1;
    static constexpr int DistanceMatrixPage = 2;
    static constexpr int PhylogeneticTreePage = 3;
    static constexpr int GeneExpressionPage = 4;
    static constexpr int ExpressionConfigurationPage = 5;
    static constexpr int ExpressionResultsPage = 6;
    static constexpr int ExpressionVolcanoPage = 7;
    static constexpr int ExpressionHeatmapPage = 8;
    static constexpr int ExpressionPCAPage = 9;

    Project currentProject;
    QStackedWidget* pages = nullptr;
    QLabel* statusLabel = nullptr;

    QListWidget* phylogeneticFileList = nullptr;
    QPushButton* openMatrixButton = nullptr;
    QPushButton* openTreeButton = nullptr;

    QStringList selectedFastaFiles;
    std::vector<std::unique_ptr<Sequence>> loadedSequences;

    QComboBox* matrixAlignmentMethodBox = nullptr;
    QTableWidget* distanceMatrixTable = nullptr;
    QLabel* matrixStatusLabel = nullptr;
    DistanceMatrix currentDistanceMatrix;

    QComboBox* treeAlignmentMethodBox = nullptr;
    PhylogeneticTreeWidget* treeGraphic = nullptr;
    QPlainTextEdit* treeOutput = nullptr;
    QLabel* treeStatusLabel = nullptr;
    DistanceMatrix treeDistanceMatrix;
    UPGMATree currentTree;

    std::unique_ptr<ExpressionDataset> expressionDataset;
    QString selectedExpressionFile;
    QLabel* expressionFileLabel = nullptr;
    QLabel* expressionSummaryLabel = nullptr;
    QTableWidget* expressionPreviewTable = nullptr;
    QPushButton* configureExpressionButton = nullptr;

    SampleGrouping sampleGrouping;
    QTableWidget* sampleGroupingTable = nullptr;
    QComboBox* normalizationMethodBox = nullptr;
    QLabel* groupingStatusLabel = nullptr;
    std::vector<QComboBox*> sampleGroupBoxes;

    std::vector<DifferentialExpressionResult> expressionResults;
    std::vector<DifferentialExpressionResult> classifiedExpressionResults;
    std::vector<DifferentialExpressionResult> filteredExpressionResults;
    std::vector<std::vector<double>> currentNormalizedExpressionValues;
    QLabel* expressionResultsSummaryLabel = nullptr;
    QLabel* expressionFilterSummaryLabel = nullptr;
    QTableWidget* expressionResultsTable = nullptr;
    QLineEdit* geneSearchBox = nullptr;
    QComboBox* regulationFilterBox = nullptr;
    QDoubleSpinBox* adjustedPThresholdBox = nullptr;
    QDoubleSpinBox* foldChangeThresholdBox = nullptr;
    QComboBox* maximumResultsBox = nullptr;
    VolcanoPlotWidget* volcanoPlotWidget = nullptr;
    ExpressionHeatmapWidget* expressionHeatmapWidget = nullptr;
    QLabel* expressionHeatmapSummaryLabel = nullptr;
    PCAPlotWidget* pcaPlotWidget = nullptr;
    QLabel* pcaSummaryLabel = nullptr;

    QWidget* createDashboardPage();
    QWidget* createPhylogeneticSetupPage();
    QWidget* createDistanceMatrixPage();
    QWidget* createPhylogeneticTreePage();
    QWidget* createGeneExpressionPage();
    QWidget* createExpressionConfigurationPage();
    QWidget* createExpressionResultsPage();
    QWidget* createExpressionVolcanoPage();
    QWidget* createExpressionHeatmapPage();
    QWidget* createExpressionPCAPage();

    void selectPhylogeneticWorkspace();
    void selectGeneExpressionWorkspace();

    void openDistanceMatrixPage();
    void openPhylogeneticTreePage();
    void openExpressionConfigurationPage();
    void openExpressionVolcanoPage();
    void openExpressionHeatmapPage();
    void openExpressionPCAPage();

    void returnToDashboard();
    void returnToPhylogeneticSetup();
    void returnToGeneExpressionSetup();
    void returnToExpressionResults();

    void importFastaFiles();
    void importExpressionFile();

    void generateDistanceMatrix();
    void generatePhylogeneticTree();
    void runDifferentialExpressionAnalysis();

    void exportMatrixResults();
    void exportTreeResults();
    void exportExpressionTables();
    void exportWidgetImage(
        QWidget* widget,
        const QString& suggestedFileName,
        const QString& dialogTitle
    );

    void populateDistanceMatrixTable(
        const DistanceMatrix& matrix
    );

    void populateSampleGroupingTable();
    void populateExpressionResultsTable();
    void applyExpressionFilters();
    void resetExpressionFilters();

    ExpressionFilterSettings
    getCurrentExpressionFilterSettings() const;

    std::unique_ptr<PairwiseAligner> createAligner(
        const QString& methodName
    ) const;
};
