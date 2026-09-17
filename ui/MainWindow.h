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
#include "gene_expression/EnrichmentResult.h"
#include "gene_expression/SampleGrouping.h"
#include "phylogenetics/DistanceMatrix.h"
#include "phylogenetics/Sequence.h"
#include "phylogenetics/UPGMATree.h"
#include "quality/QualityReport.h"
#include "session/ProjectSession.h"

class QLabel;
class QListWidget;
class QComboBox;
class QCloseEvent;
class QDoubleSpinBox;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QStackedWidget;
class QTableWidget;
class QWidget;

class PairwiseAligner;
class ExpressionHeatmapWidget;
class EnrichmentBarChartWidget;
class PCAPlotWidget;
class PhylogeneticTreeWidget;
class VolcanoPlotWidget;
class WorkflowCanvasWidget;

class MainWindow : public QMainWindow
{
public:
    explicit MainWindow(QWidget* parent = nullptr);

protected:
    void closeEvent(QCloseEvent* event) override;

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
    static constexpr int PhylogeneticQualityPage = 10;
    static constexpr int ExpressionQualityPage = 11;
    static constexpr int ExpressionEnrichmentPage = 12;
    static constexpr int WorkflowBuilderPage = 13;
    static constexpr int ProjectHistoryPage = 14;

    Project currentProject;
    QStackedWidget* pages = nullptr;
    QLabel* statusLabel = nullptr;

    QListWidget* phylogeneticFileList = nullptr;
    QPushButton* openMatrixButton = nullptr;
    QPushButton* openTreeButton = nullptr;
    QPushButton* phylogeneticQualityButton = nullptr;

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
    QPushButton* expressionQualityButton = nullptr;

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

    std::vector<EnrichmentResult> enrichmentResults;
    std::vector<EnrichmentResult> filteredEnrichmentResults;
    QTableWidget* enrichmentResultsTable = nullptr;
    QLabel* enrichmentSummaryLabel = nullptr;
    QLineEdit* pathwaySearchBox = nullptr;
    QComboBox* pathwayCategoryBox = nullptr;
    QDoubleSpinBox* enrichmentPThresholdBox = nullptr;
    QComboBox* maximumPathwaysBox = nullptr;
    EnrichmentBarChartWidget* enrichmentBarChart = nullptr;

    QTableWidget* phylogeneticQualityTable = nullptr;
    QLabel* phylogeneticQualitySummaryLabel = nullptr;
    QTableWidget* expressionQualityTable = nullptr;
    QLabel* expressionQualitySummaryLabel = nullptr;

    WorkflowCanvasWidget* workflowCanvas = nullptr;
    QComboBox* workflowTemplateBox = nullptr;
    QComboBox* workflowNodeTypeBox = nullptr;
    QLabel* workflowSelectionLabel = nullptr;
    QLabel* workflowValidationLabel = nullptr;
    QPushButton* openSelectedWorkflowStepButton = nullptr;

    ProjectSession projectSession;
    QString currentProjectFile;
    bool projectModified = false;
    QTableWidget* projectHistoryTable = nullptr;
    QLabel* projectHistorySummaryLabel = nullptr;

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
    QWidget* createPhylogeneticQualityPage();
    QWidget* createExpressionQualityPage();
    QWidget* createExpressionEnrichmentPage();
    QWidget* createWorkflowBuilderPage();
    QWidget* createProjectHistoryPage();

    void selectPhylogeneticWorkspace();
    void selectGeneExpressionWorkspace();

    void openDistanceMatrixPage();
    void openPhylogeneticTreePage();
    void openExpressionConfigurationPage();
    void openExpressionVolcanoPage();
    void openExpressionHeatmapPage();
    void openExpressionPCAPage();
    void openPhylogeneticQualityPage();
    void openExpressionQualityPage();
    void openExpressionEnrichmentPage();
    void openWorkflowBuilderPage();
    void openProjectHistoryPage();

    void returnToDashboard();
    void returnToPhylogeneticSetup();
    void returnToGeneExpressionSetup();
    void returnToExpressionResults();

    void importFastaFiles();
    void importExpressionFile();
    void loadFastaFiles(
        const QStringList& filePaths,
        bool recordHistory
    );
    void loadExpressionFile(
        const QString& filePath,
        bool recordHistory
    );

    void generateDistanceMatrix();
    void generatePhylogeneticTree();
    void runDifferentialExpressionAnalysis();
    void runFunctionalEnrichmentAnalysis();

    void exportMatrixResults();
    void exportTreeResults();
    void exportExpressionTables();
    void exportEnrichmentResults();
    void generateExpressionHtmlReport();
    void generatePhylogeneticHtmlReport();
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
    void populateQualityReportTable(
        QTableWidget* table,
        QLabel* summaryLabel,
        const QualityReport& report
    );
    void applyExpressionFilters();
    void resetExpressionFilters();
    void applyEnrichmentFilters();
    void resetEnrichmentFilters();
    void populateEnrichmentResultsTable();
    void loadWorkflowTemplate();
    void refreshWorkflowNodeStates();
    void openSelectedWorkflowStep();
    void saveProject();
    void loadProject();
    void showAboutDialog();
    void updateWindowTitle();
    bool confirmDiscardUnsavedChanges();
    void markProjectModified();
    void refreshProjectHistoryTable();
    void recordHistory(
        const QString& workspace,
        const QString& action,
        const QString& status,
        const QString& details
    );
    bool saveReportImage(
        QWidget* widget,
        const QString& filePath,
        int width = 1200,
        int height = 700
    );

    ExpressionFilterSettings
    getCurrentExpressionFilterSettings() const;

    std::unique_ptr<PairwiseAligner> createAligner(
        const QString& methodName
    ) const;
};
