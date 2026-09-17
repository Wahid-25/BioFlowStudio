#include "MainWindow.h"

#include <QAbstractItemView>
#include <QAction>
#include <QApplication>
#include <QColor>
#include <QComboBox>
#include <QDir>
#include <QDateTime>
#include <QDesktopServices>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QGridLayout>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPixmap>
#include <QPainter>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QStackedWidget>
#include <QStringList>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <QWidget>
#include <QUrl>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>

#include <algorithm>
#include <exception>
#include <fstream>
#include <iomanip>
#include <memory>
#include <stdexcept>
#include <utility>

#include "export/PhylogeneticExporter.h"
#include "export/GeneExpressionExporter.h"
#include "gene_expression/NormalizationStrategy.h"
#include "gene_expression/PCAAnalyzer.h"
#include "gene_expression/ExpressionParser.h"
#include "gene_expression/HypergeometricEnrichmentAnalyzer.h"
#include "gene_expression/PathwayDatabase.h"
#include "gene_expression/WelchTTestAnalyzer.h"
#include "phylogenetics/FastaParser.h"
#include "phylogenetics/PairwiseAligner.h"
#include "quality/ExpressionQualityAnalyzer.h"
#include "quality/SequenceQualityAnalyzer.h"
#include "report/HtmlReportGenerator.h"
#include "session/ProjectSerializer.h"
#include "visualization/PhylogeneticTreeWidget.h"
#include "visualization/ExpressionHeatmapWidget.h"
#include "visualization/EnrichmentBarChartWidget.h"
#include "visualization/PCAPlotWidget.h"
#include "visualization/VolcanoPlotWidget.h"
#include "visualization/WorkflowCanvasWidget.h"
#include "workflow/WorkflowTemplateFactory.h"

namespace
{
class NumericTableWidgetItem : public QTableWidgetItem
{
public:
    NumericTableWidgetItem(double value, int precision)
        : QTableWidgetItem(QString::number(value, 'g', precision)),
          numericValue(value)
    {
        setTextAlignment(Qt::AlignCenter);
    }

    bool operator<(const QTableWidgetItem& other) const override
    {
        const auto* numericOther =
            dynamic_cast<const NumericTableWidgetItem*>(&other);

        if (numericOther != nullptr)
        {
            return numericValue < numericOther->numericValue;
        }

        return QTableWidgetItem::operator<(other);
    }

private:
    double numericValue;
};

QLabel* createPageTitle(const QString& text)
{
    QLabel* label = new QLabel(text);

    label->setAlignment(Qt::AlignCenter);
    label->setStyleSheet(
        "font-size: 30px;"
        "font-weight: bold;"
        "color: #163A5F;"
    );

    return label;
}

QLabel* createDescription(const QString& text)
{
    QLabel* label = new QLabel(text);

    label->setAlignment(Qt::AlignCenter);
    label->setWordWrap(true);
    label->setStyleSheet(
        "color: #40566B;"
        "font-size: 15px;"
    );

    return label;
}
}

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      currentProject(
          "BioFlow Demonstration",
          "A workflow platform for biological data analysis",
          WorkspaceType::NotSelected
      )
{
    setWindowTitle("BioFlow Studio");
    resize(1100, 700);
    setMinimumSize(900, 600);

    pages = new QStackedWidget;

    pages->addWidget(createDashboardPage());
    pages->addWidget(createPhylogeneticSetupPage());
    pages->addWidget(createDistanceMatrixPage());
    pages->addWidget(createPhylogeneticTreePage());
    pages->addWidget(createGeneExpressionPage());
    pages->addWidget(createExpressionConfigurationPage());
    pages->addWidget(createExpressionResultsPage());
    pages->addWidget(createExpressionVolcanoPage());
    pages->addWidget(createExpressionHeatmapPage());
    pages->addWidget(createExpressionPCAPage());
    pages->addWidget(createPhylogeneticQualityPage());
    pages->addWidget(createExpressionQualityPage());
    pages->addWidget(createExpressionEnrichmentPage());
    pages->addWidget(createWorkflowBuilderPage());
    pages->addWidget(createProjectHistoryPage());

    pages->setCurrentIndex(DashboardPage);
    setCentralWidget(pages);

    QMenu* fileMenu = menuBar()->addMenu("File");
    QAction* openProjectAction = fileMenu->addAction("Open Project...");
    QAction* saveProjectAction = fileMenu->addAction("Save Project...");
    fileMenu->addSeparator();
    QAction* exitAction = fileMenu->addAction("Exit");

    QMenu* viewMenu = menuBar()->addMenu("View");
    QAction* historyAction = viewMenu->addAction("Analysis History");

    connect(
        openProjectAction,
        &QAction::triggered,
        this,
        &MainWindow::loadProject
    );
    connect(
        saveProjectAction,
        &QAction::triggered,
        this,
        &MainWindow::saveProject
    );
    connect(
        historyAction,
        &QAction::triggered,
        this,
        &MainWindow::openProjectHistoryPage
    );
    connect(exitAction, &QAction::triggered, this, &QWidget::close);

    setStyleSheet(
        R"(
        QWidget {
            background-color: #F4F7FA;
            color: #203040;
            font-family: Arial;
            font-size: 15px;
        }

        #titleLabel {
            color: #163A5F;
            font-size: 38px;
            font-weight: bold;
        }

        #subtitleLabel {
            color: #40566B;
            font-size: 20px;
        }

        #descriptionLabel {
            color: #40566B;
            font-size: 16px;
        }

        #statusLabel {
            color: #2D6A4F;
            font-size: 16px;
            font-weight: bold;
        }

        QPushButton {
            background-color: #163A5F;
            color: white;
            border: none;
            border-radius: 8px;
            font-size: 16px;
            font-weight: bold;
            padding: 10px;
        }

        QPushButton:hover {
            background-color: #245C8A;
        }

        QPushButton:pressed {
            background-color: #0F2B47;
        }

        QPushButton:disabled {
            background-color: #9AA9B7;
            color: #E5EBF0;
        }

        QListWidget {
            background-color: white;
            color: #203040;
            border: 1px solid #D7E0E8;
            border-radius: 6px;
            padding: 8px;
        }

        QComboBox {
            background-color: white;
            color: #203040;
            border: 1px solid #BCC9D4;
            border-radius: 5px;
            padding: 8px;
        }

        QTableWidget {
            background-color: white;
            alternate-background-color: #EDF3F8;
            color: #203040;
            border: 1px solid #D7E0E8;
            gridline-color: #CBD5DF;
        }

        QHeaderView::section {
            background-color: #E3EBF2;
            color: #163A5F;
            font-weight: bold;
            padding: 6px;
            border: 1px solid #CBD5DF;
        }

        QPlainTextEdit {
            background-color: white;
            color: #203040;
            border: 1px solid #D7E0E8;
            border-radius: 5px;
            font-family: Consolas;
            font-size: 13px;
        }
        )"
    );
}

QWidget* MainWindow::createDashboardPage()
{
    QWidget* page = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(page);

    layout->setContentsMargins(90, 60, 90, 60);
    layout->setSpacing(20);

    QLabel* title = new QLabel("BioFlow Studio");
    title->setObjectName("titleLabel");
    title->setAlignment(Qt::AlignCenter);

    QLabel* subtitle = new QLabel(
        "Object-Oriented Bioinformatics Workflow Platform"
    );

    subtitle->setObjectName("subtitleLabel");
    subtitle->setAlignment(Qt::AlignCenter);

    QLabel* description = new QLabel(
        "Choose a biological analysis workspace."
    );

    description->setObjectName("descriptionLabel");
    description->setAlignment(Qt::AlignCenter);

    QPushButton* phylogeneticButton =
        new QPushButton("Phylogenetic Analysis");

    QPushButton* expressionButton =
        new QPushButton("Gene Expression Analysis");

    QPushButton* workflowButton =
        new QPushButton("Graphical Workflow Builder");

    phylogeneticButton->setMinimumHeight(62);
    expressionButton->setMinimumHeight(62);
    workflowButton->setMinimumHeight(62);

    statusLabel = new QLabel("No workspace selected");
    statusLabel->setObjectName("statusLabel");
    statusLabel->setAlignment(Qt::AlignCenter);

    layout->addStretch();
    layout->addWidget(title);
    layout->addWidget(subtitle);
    layout->addWidget(description);
    layout->addSpacing(15);
    layout->addWidget(phylogeneticButton);
    layout->addWidget(expressionButton);
    layout->addWidget(workflowButton);
    layout->addWidget(statusLabel);
    layout->addStretch();

    connect(
        phylogeneticButton,
        &QPushButton::clicked,
        this,
        &MainWindow::selectPhylogeneticWorkspace
    );

    connect(
        expressionButton,
        &QPushButton::clicked,
        this,
        &MainWindow::selectGeneExpressionWorkspace
    );

    connect(
        workflowButton,
        &QPushButton::clicked,
        this,
        &MainWindow::openWorkflowBuilderPage
    );

    return page;
}

QWidget* MainWindow::createPhylogeneticSetupPage()
{
    QWidget* page = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(page);

    layout->setContentsMargins(70, 35, 70, 35);
    layout->setSpacing(14);

    QLabel* title =
        createPageTitle("Phylogenetic Analysis");

    QLabel* description = createDescription(
        "Import one or more FASTA files. After validation, "
        "choose which phylogenetic analysis to perform."
    );

    QPushButton* importButton =
        new QPushButton("Select FASTA Files");

    importButton->setMinimumHeight(50);

    phylogeneticFileList = new QListWidget;
    phylogeneticFileList->setMinimumHeight(250);
    phylogeneticFileList->addItem(
        "No FASTA sequences imported."
    );

    QLabel* optionsLabel =
        new QLabel("Choose an analysis:");

    optionsLabel->setStyleSheet(
        "font-size: 17px;"
        "font-weight: bold;"
        "color: #163A5F;"
    );

    optionsLabel->setAlignment(Qt::AlignCenter);

    openMatrixButton =
        new QPushButton(
            "Distance Matrix and Heatmap"
        );

    openTreeButton =
        new QPushButton(
            "Phylogenetic Tree Analysis"
        );

    phylogeneticQualityButton =
        new QPushButton(
            "Data Quality Dashboard"
        );

    openMatrixButton->setMinimumHeight(58);
    openTreeButton->setMinimumHeight(58);
    phylogeneticQualityButton->setMinimumHeight(58);

    openMatrixButton->setEnabled(false);
    openTreeButton->setEnabled(false);
    phylogeneticQualityButton->setEnabled(false);

    QHBoxLayout* analysisOptions =
        new QHBoxLayout;

    analysisOptions->setSpacing(12);
    analysisOptions->addWidget(openMatrixButton);
    analysisOptions->addWidget(openTreeButton);
    analysisOptions->addWidget(phylogeneticQualityButton);

    QPushButton* backButton =
        new QPushButton("Back to Dashboard");

    backButton->setMinimumHeight(45);

    layout->addWidget(title);
    layout->addWidget(description);
    layout->addWidget(importButton);
    layout->addWidget(phylogeneticFileList, 1);
    layout->addWidget(optionsLabel);
    layout->addLayout(analysisOptions);
    layout->addWidget(backButton);

    connect(
        importButton,
        &QPushButton::clicked,
        this,
        &MainWindow::importFastaFiles
    );

    connect(
        openMatrixButton,
        &QPushButton::clicked,
        this,
        &MainWindow::openDistanceMatrixPage
    );

    connect(
        openTreeButton,
        &QPushButton::clicked,
        this,
        &MainWindow::openPhylogeneticTreePage
    );

    connect(
        phylogeneticQualityButton,
        &QPushButton::clicked,
        this,
        &MainWindow::openPhylogeneticQualityPage
    );

    connect(
        backButton,
        &QPushButton::clicked,
        this,
        &MainWindow::returnToDashboard
    );

    return page;
}

QWidget* MainWindow::createDistanceMatrixPage()
{
    QWidget* page = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(page);

    layout->setContentsMargins(35, 25, 35, 25);
    layout->setSpacing(10);

    QLabel* title =
        createPageTitle(
            "Distance Matrix and Heatmap"
        );

    QLabel* description = createDescription(
        "Choose a pairwise strategy and calculate "
        "normalized distances between every sequence."
    );

    QLabel* methodLabel =
        new QLabel("Pairwise distance method:");

    methodLabel->setStyleSheet(
        "font-weight: bold;"
        "color: #163A5F;"
    );

    matrixAlignmentMethodBox = new QComboBox;

    matrixAlignmentMethodBox->addItem(
        "Needleman-Wunsch Global Distance"
    );

    matrixAlignmentMethodBox->addItem(
        "Hamming Distance"
    );

    QPushButton* generateButton =
        new QPushButton("Generate Distance Matrix");

    QPushButton* exportButton =
        new QPushButton("Export CSV and PHYLIP");

    QPushButton* reportButton =
        new QPushButton("Generate HTML Report");

    generateButton->setMinimumHeight(45);
    exportButton->setMinimumHeight(45);
    reportButton->setMinimumHeight(45);

    QHBoxLayout* actionLayout =
        new QHBoxLayout;

    actionLayout->setSpacing(10);
    actionLayout->addWidget(generateButton);
    actionLayout->addWidget(exportButton);
    actionLayout->addWidget(reportButton);

    matrixStatusLabel =
        new QLabel("No distance matrix generated");

    matrixStatusLabel->setAlignment(Qt::AlignCenter);
    matrixStatusLabel->setStyleSheet(
        "font-weight: bold;"
        "color: #40566B;"
    );

    distanceMatrixTable = new QTableWidget;

    distanceMatrixTable->setEditTriggers(
        QAbstractItemView::NoEditTriggers
    );

    distanceMatrixTable->setAlternatingRowColors(true);

    QPushButton* backButton =
        new QPushButton(
            "Back to Phylogenetic Setup"
        );

    backButton->setMinimumHeight(45);

    layout->addWidget(title);
    layout->addWidget(description);
    layout->addWidget(methodLabel);
    layout->addWidget(matrixAlignmentMethodBox);
    layout->addLayout(actionLayout);
    layout->addWidget(matrixStatusLabel);
    layout->addWidget(distanceMatrixTable, 1);
    layout->addWidget(backButton);

    connect(
        generateButton,
        &QPushButton::clicked,
        this,
        &MainWindow::generateDistanceMatrix
    );

    connect(
        exportButton,
        &QPushButton::clicked,
        this,
        &MainWindow::exportMatrixResults
    );

    connect(
        reportButton,
        &QPushButton::clicked,
        this,
        &MainWindow::generatePhylogeneticHtmlReport
    );

    connect(
        backButton,
        &QPushButton::clicked,
        this,
        &MainWindow::returnToPhylogeneticSetup
    );

    return page;
}

QWidget* MainWindow::createPhylogeneticTreePage()
{
    QWidget* page = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(page);

    layout->setContentsMargins(35, 25, 35, 25);
    layout->setSpacing(9);

    QLabel* title =
        createPageTitle(
            "UPGMA Phylogenetic Tree"
        );

    QLabel* description = createDescription(
        "The selected distance algorithm is calculated "
        "automatically before constructing the tree."
    );

    QLabel* methodLabel =
        new QLabel("Tree distance method:");

    methodLabel->setStyleSheet(
        "font-weight: bold;"
        "color: #163A5F;"
    );

    treeAlignmentMethodBox = new QComboBox;

    treeAlignmentMethodBox->addItem(
        "Needleman-Wunsch Global Distance"
    );

    treeAlignmentMethodBox->addItem(
        "Hamming Distance"
    );

    QPushButton* generateButton =
        new QPushButton("Generate UPGMA Tree");

    QPushButton* exportButton =
        new QPushButton("Export Newick and PNG");

    QPushButton* reportButton =
        new QPushButton("Generate HTML Report");

    generateButton->setMinimumHeight(45);
    exportButton->setMinimumHeight(45);
    reportButton->setMinimumHeight(45);

    QHBoxLayout* actionLayout =
        new QHBoxLayout;

    actionLayout->setSpacing(10);
    actionLayout->addWidget(generateButton);
    actionLayout->addWidget(exportButton);
    actionLayout->addWidget(reportButton);

    treeStatusLabel =
        new QLabel("No phylogenetic tree generated");

    treeStatusLabel->setAlignment(Qt::AlignCenter);
    treeStatusLabel->setStyleSheet(
        "font-weight: bold;"
        "color: #40566B;"
    );

    treeGraphic =
        new PhylogeneticTreeWidget;

    treeGraphic->setMinimumHeight(300);

    treeOutput = new QPlainTextEdit;

    treeOutput->setReadOnly(true);
    treeOutput->setMaximumHeight(75);
    treeOutput->setPlaceholderText(
        "Newick representation will appear here."
    );

    QPushButton* backButton =
        new QPushButton(
            "Back to Phylogenetic Setup"
        );

    backButton->setMinimumHeight(45);

    layout->addWidget(title);
    layout->addWidget(description);
    layout->addWidget(methodLabel);
    layout->addWidget(treeAlignmentMethodBox);
    layout->addLayout(actionLayout);
    layout->addWidget(treeStatusLabel);
    layout->addWidget(treeGraphic, 1);
    layout->addWidget(treeOutput);
    layout->addWidget(backButton);

    connect(
        generateButton,
        &QPushButton::clicked,
        this,
        &MainWindow::generatePhylogeneticTree
    );

    connect(
        exportButton,
        &QPushButton::clicked,
        this,
        &MainWindow::exportTreeResults
    );

    connect(
        reportButton,
        &QPushButton::clicked,
        this,
        &MainWindow::generatePhylogeneticHtmlReport
    );

    connect(
        backButton,
        &QPushButton::clicked,
        this,
        &MainWindow::returnToPhylogeneticSetup
    );

    return page;
}

QWidget* MainWindow::createGeneExpressionPage()
{
    QWidget* page = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(page);

    layout->setContentsMargins(45, 25, 45, 25);
    layout->setSpacing(10);

    QLabel* title =
        createPageTitle(
            "Gene Expression Analysis"
        );

    QLabel* description = createDescription(
        "Import a CSV or TSV expression dataset. "
        "Genes must be rows and biological samples "
        "must be columns."
    );

    QPushButton* importButton =
        new QPushButton(
            "Select Expression File"
        );

    importButton->setMinimumHeight(48);

    expressionFileLabel =
        new QLabel("No expression file selected");

    expressionFileLabel->setAlignment(
        Qt::AlignCenter
    );

    expressionFileLabel->setWordWrap(true);

    expressionFileLabel->setStyleSheet(
        "background-color: white;"
        "color: #40566B;"
        "border: 1px solid #D7E0E8;"
        "border-radius: 6px;"
        "padding: 10px;"
    );

    expressionSummaryLabel =
        new QLabel(
            "Import a dataset to view its summary."
        );

    expressionSummaryLabel->setAlignment(
        Qt::AlignCenter
    );

    expressionSummaryLabel->setStyleSheet(
        "font-weight: bold;"
        "color: #40566B;"
    );

    expressionPreviewTable =
        new QTableWidget;

    expressionPreviewTable->setEditTriggers(
        QAbstractItemView::NoEditTriggers
    );

    expressionPreviewTable->setAlternatingRowColors(
        true
    );

    configureExpressionButton =
        new QPushButton(
            "Configure Differential Expression Analysis"
        );

    configureExpressionButton->setMinimumHeight(48);
    configureExpressionButton->setEnabled(false);

    expressionQualityButton =
        new QPushButton("View Data Quality Dashboard");
    expressionQualityButton->setMinimumHeight(48);
    expressionQualityButton->setEnabled(false);

    QHBoxLayout* expressionActionLayout = new QHBoxLayout;
    expressionActionLayout->addWidget(expressionQualityButton);
    expressionActionLayout->addWidget(configureExpressionButton);

    QPushButton* backButton =
        new QPushButton("Back to Dashboard");

    backButton->setMinimumHeight(45);

    layout->addWidget(title);
    layout->addWidget(description);
    layout->addWidget(importButton);
    layout->addWidget(expressionFileLabel);
    layout->addWidget(expressionSummaryLabel);
    layout->addWidget(expressionPreviewTable, 1);
    layout->addLayout(expressionActionLayout);
    layout->addWidget(backButton);

    connect(
        importButton,
        &QPushButton::clicked,
        this,
        &MainWindow::importExpressionFile
    );

    connect(
        configureExpressionButton,
        &QPushButton::clicked,
        this,
        &MainWindow::openExpressionConfigurationPage
    );

    connect(
        expressionQualityButton,
        &QPushButton::clicked,
        this,
        &MainWindow::openExpressionQualityPage
    );

    connect(
        backButton,
        &QPushButton::clicked,
        this,
        &MainWindow::returnToDashboard
    );

    return page;
}

QWidget* MainWindow::createExpressionConfigurationPage()
{
    QWidget* page = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(page);

    layout->setContentsMargins(45, 25, 45, 25);
    layout->setSpacing(10);

    QLabel* title = createPageTitle(
        "Configure Differential Expression Analysis"
    );

    QLabel* description = createDescription(
        "Assign each sample to Control or Treatment, choose a "
        "normalization strategy, and run Welch's independent t-test."
    );

    sampleGroupingTable = new QTableWidget;
    sampleGroupingTable->setColumnCount(2);
    sampleGroupingTable->setHorizontalHeaderLabels(
        {"Sample", "Experimental Group"}
    );
    sampleGroupingTable->setEditTriggers(
        QAbstractItemView::NoEditTriggers
    );
    sampleGroupingTable->setAlternatingRowColors(true);
    sampleGroupingTable->horizontalHeader()->setSectionResizeMode(
        QHeaderView::Stretch
    );
    sampleGroupingTable->verticalHeader()->setVisible(false);

    QLabel* normalizationLabel = new QLabel(
        "Normalization strategy:"
    );
    normalizationLabel->setStyleSheet(
        "font-weight: bold; color: #163A5F;"
    );

    normalizationMethodBox = new QComboBox;
    normalizationMethodBox->addItems(
        {
            "Raw values (no transformation)",
            "Log2 transformation: log2(x + 1)",
            "Z-score normalization per gene"
        }
    );
    normalizationMethodBox->setMinimumHeight(40);

    groupingStatusLabel = new QLabel(
        "Import a dataset before configuring the analysis."
    );
    groupingStatusLabel->setAlignment(Qt::AlignCenter);
    groupingStatusLabel->setWordWrap(true);
    groupingStatusLabel->setStyleSheet(
        "font-weight: bold; color: #40566B;"
    );

    QPushButton* runButton = new QPushButton(
        "Run Differential Expression Analysis"
    );
    runButton->setMinimumHeight(48);

    QPushButton* backButton = new QPushButton(
        "Back to Expression Dataset"
    );
    backButton->setMinimumHeight(44);

    layout->addWidget(title);
    layout->addWidget(description);
    layout->addWidget(sampleGroupingTable, 1);
    layout->addWidget(normalizationLabel);
    layout->addWidget(normalizationMethodBox);
    layout->addWidget(groupingStatusLabel);
    layout->addWidget(runButton);
    layout->addWidget(backButton);

    connect(
        runButton,
        &QPushButton::clicked,
        this,
        &MainWindow::runDifferentialExpressionAnalysis
    );

    connect(
        backButton,
        &QPushButton::clicked,
        this,
        &MainWindow::returnToGeneExpressionSetup
    );

    return page;
}

QWidget* MainWindow::createExpressionResultsPage()
{
    QWidget* page = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(page);

    layout->setContentsMargins(35, 22, 35, 22);
    layout->setSpacing(10);

    QLabel* title = createPageTitle(
        "Differential Expression Results"
    );

    QLabel* description = createDescription(
        "Genes are classified using adjusted p-value < 0.05 and "
        "absolute log2 fold change >= 1. Click a column heading to sort."
    );

    expressionResultsSummaryLabel = new QLabel(
        "Run an analysis to generate results."
    );
    expressionResultsSummaryLabel->setAlignment(Qt::AlignCenter);
    expressionResultsSummaryLabel->setWordWrap(true);
    expressionResultsSummaryLabel->setStyleSheet(
        "font-weight: bold; color: #40566B;"
    );

    QGridLayout* filterLayout = new QGridLayout;
    filterLayout->setHorizontalSpacing(10);
    filterLayout->setVerticalSpacing(7);

    QLabel* searchLabel = new QLabel("Search gene:");
    searchLabel->setStyleSheet("font-weight: bold;");

    geneSearchBox = new QLineEdit;
    geneSearchBox->setPlaceholderText("Example: TP53");
    geneSearchBox->setClearButtonEnabled(true);

    QLabel* regulationLabel = new QLabel("Regulation:");
    regulationLabel->setStyleSheet("font-weight: bold;");

    regulationFilterBox = new QComboBox;
    regulationFilterBox->addItems(
        {
            "All genes",
            "Upregulated",
            "Downregulated",
            "Not significant"
        }
    );

    QLabel* maximumLabel = new QLabel("Display:");
    maximumLabel->setStyleSheet("font-weight: bold;");

    maximumResultsBox = new QComboBox;
    maximumResultsBox->addItem("All results", 0);
    maximumResultsBox->addItem("Top 10", 10);
    maximumResultsBox->addItem("Top 20", 20);
    maximumResultsBox->addItem("Top 50", 50);

    QLabel* pValueLabel = new QLabel("Adjusted p-value <=");
    pValueLabel->setStyleSheet("font-weight: bold;");

    adjustedPThresholdBox = new QDoubleSpinBox;
    adjustedPThresholdBox->setRange(0.000001, 1.0);
    adjustedPThresholdBox->setDecimals(6);
    adjustedPThresholdBox->setSingleStep(0.01);
    adjustedPThresholdBox->setValue(0.05);

    QLabel* foldChangeLabel = new QLabel("Minimum |log2 FC|:");
    foldChangeLabel->setStyleSheet("font-weight: bold;");

    foldChangeThresholdBox = new QDoubleSpinBox;
    foldChangeThresholdBox->setRange(0.0, 20.0);
    foldChangeThresholdBox->setDecimals(2);
    foldChangeThresholdBox->setSingleStep(0.25);
    foldChangeThresholdBox->setValue(1.0);

    QPushButton* resetFiltersButton = new QPushButton(
        "Reset Filters"
    );
    resetFiltersButton->setMinimumHeight(36);

    expressionFilterSummaryLabel = new QLabel(
        "Run an analysis to enable interactive filtering."
    );
    expressionFilterSummaryLabel->setAlignment(Qt::AlignCenter);
    expressionFilterSummaryLabel->setWordWrap(true);
    expressionFilterSummaryLabel->setStyleSheet(
        "font-weight: bold; color: #40566B;"
    );

    filterLayout->addWidget(searchLabel, 0, 0);
    filterLayout->addWidget(geneSearchBox, 0, 1);
    filterLayout->addWidget(regulationLabel, 0, 2);
    filterLayout->addWidget(regulationFilterBox, 0, 3);
    filterLayout->addWidget(maximumLabel, 0, 4);
    filterLayout->addWidget(maximumResultsBox, 0, 5);
    filterLayout->addWidget(pValueLabel, 1, 0);
    filterLayout->addWidget(adjustedPThresholdBox, 1, 1);
    filterLayout->addWidget(foldChangeLabel, 1, 2);
    filterLayout->addWidget(foldChangeThresholdBox, 1, 3);
    filterLayout->addWidget(resetFiltersButton, 1, 4, 1, 2);
    filterLayout->addWidget(expressionFilterSummaryLabel, 2, 0, 1, 6);

    expressionResultsTable = new QTableWidget;
    expressionResultsTable->setColumnCount(7);
    expressionResultsTable->setHorizontalHeaderLabels(
        {
            "Gene",
            "Control Mean",
            "Treatment Mean",
            "Log2 Fold Change",
            "P-value",
            "Adjusted P-value",
            "Regulation"
        }
    );
    expressionResultsTable->setEditTriggers(
        QAbstractItemView::NoEditTriggers
    );
    expressionResultsTable->setSelectionBehavior(
        QAbstractItemView::SelectRows
    );
    expressionResultsTable->setAlternatingRowColors(true);
    expressionResultsTable->verticalHeader()->setVisible(false);
    expressionResultsTable->horizontalHeader()->setSectionResizeMode(
        QHeaderView::ResizeToContents
    );
    expressionResultsTable->horizontalHeader()->setStretchLastSection(true);

    QPushButton* backButton = new QPushButton(
        "Back to Analysis Configuration"
    );
    backButton->setMinimumHeight(44);

    QPushButton* volcanoButton = new QPushButton(
        "Open Interactive Volcano Plot"
    );
    volcanoButton->setMinimumHeight(46);

    QPushButton* heatmapButton = new QPushButton(
        "Open Gene-Expression Heatmap"
    );
    heatmapButton->setMinimumHeight(46);

    QPushButton* pcaButton = new QPushButton(
        "Open PCA Sample Plot"
    );
    pcaButton->setMinimumHeight(46);

    QHBoxLayout* visualizationButtonLayout = new QHBoxLayout;
    visualizationButtonLayout->addWidget(volcanoButton);
    visualizationButtonLayout->addWidget(heatmapButton);
    visualizationButtonLayout->addWidget(pcaButton);

    QPushButton* enrichmentButton = new QPushButton(
        "Open Functional Enrichment and Pathway Analysis"
    );
    enrichmentButton->setMinimumHeight(46);

    QPushButton* exportTablesButton = new QPushButton(
        "Export Analysis Tables and Summary"
    );
    exportTablesButton->setMinimumHeight(46);

    QPushButton* reportButton = new QPushButton(
        "Generate Complete HTML Report"
    );
    reportButton->setMinimumHeight(46);

    QHBoxLayout* resultActionLayout = new QHBoxLayout;
    resultActionLayout->addWidget(exportTablesButton);
    resultActionLayout->addWidget(reportButton);

    layout->addWidget(title);
    layout->addWidget(description);
    layout->addWidget(expressionResultsSummaryLabel);
    layout->addLayout(filterLayout);
    layout->addWidget(expressionResultsTable, 1);
    layout->addLayout(visualizationButtonLayout);
    layout->addWidget(enrichmentButton);
    layout->addLayout(resultActionLayout);
    layout->addWidget(backButton);

    connect(
        geneSearchBox,
        &QLineEdit::textChanged,
        this,
        [this]() { applyExpressionFilters(); }
    );

    connect(
        regulationFilterBox,
        &QComboBox::currentIndexChanged,
        this,
        [this]() { applyExpressionFilters(); }
    );

    connect(
        maximumResultsBox,
        &QComboBox::currentIndexChanged,
        this,
        [this]() { applyExpressionFilters(); }
    );

    connect(
        adjustedPThresholdBox,
        &QDoubleSpinBox::valueChanged,
        this,
        [this]() { applyExpressionFilters(); }
    );

    connect(
        foldChangeThresholdBox,
        &QDoubleSpinBox::valueChanged,
        this,
        [this]() { applyExpressionFilters(); }
    );

    connect(
        resetFiltersButton,
        &QPushButton::clicked,
        this,
        &MainWindow::resetExpressionFilters
    );

    connect(
        volcanoButton,
        &QPushButton::clicked,
        this,
        &MainWindow::openExpressionVolcanoPage
    );

    connect(
        heatmapButton,
        &QPushButton::clicked,
        this,
        &MainWindow::openExpressionHeatmapPage
    );

    connect(
        pcaButton,
        &QPushButton::clicked,
        this,
        &MainWindow::openExpressionPCAPage
    );

    connect(
        enrichmentButton,
        &QPushButton::clicked,
        this,
        &MainWindow::openExpressionEnrichmentPage
    );

    connect(
        exportTablesButton,
        &QPushButton::clicked,
        this,
        &MainWindow::exportExpressionTables
    );

    connect(
        reportButton,
        &QPushButton::clicked,
        this,
        &MainWindow::generateExpressionHtmlReport
    );

    connect(
        backButton,
        &QPushButton::clicked,
        this,
        &MainWindow::openExpressionConfigurationPage
    );

    return page;
}

QWidget* MainWindow::createExpressionVolcanoPage()
{
    QWidget* page = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(page);

    layout->setContentsMargins(30, 20, 30, 20);
    layout->setSpacing(10);

    QLabel* title = createPageTitle(
        "Interactive Volcano Plot"
    );

    QLabel* description = createDescription(
        "Green points are upregulated, red points are downregulated, "
        "and grey points are not significant. Hover over a point to "
        "see its gene name and values. Drag a rectangle to zoom."
    );

    volcanoPlotWidget = new VolcanoPlotWidget;

    QPushButton* resetZoomButton = new QPushButton(
        "Reset Plot Zoom"
    );
    resetZoomButton->setMinimumHeight(42);

    QPushButton* exportButton = new QPushButton(
        "Export Volcano Plot PNG"
    );
    exportButton->setMinimumHeight(42);

    QPushButton* backButton = new QPushButton(
        "Back to Differential Expression Results"
    );
    backButton->setMinimumHeight(44);

    QHBoxLayout* buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(resetZoomButton);
    buttonLayout->addWidget(exportButton);
    buttonLayout->addWidget(backButton);

    layout->addWidget(title);
    layout->addWidget(description);
    layout->addWidget(volcanoPlotWidget, 1);
    layout->addLayout(buttonLayout);

    connect(
        resetZoomButton,
        &QPushButton::clicked,
        this,
        [this]()
        {
            volcanoPlotWidget->chart()->zoomReset();
        }
    );

    connect(
        exportButton,
        &QPushButton::clicked,
        this,
        [this]()
        {
            exportWidgetImage(
                volcanoPlotWidget,
                "volcano_plot.png",
                "Export Volcano Plot"
            );
        }
    );

    connect(
        backButton,
        &QPushButton::clicked,
        this,
        &MainWindow::returnToExpressionResults
    );

    return page;
}

QWidget* MainWindow::createExpressionHeatmapPage()
{
    QWidget* page = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(page);

    layout->setContentsMargins(25, 18, 25, 18);
    layout->setSpacing(9);

    QLabel* title = createPageTitle(
        "Gene-Expression Heatmap"
    );

    QLabel* description = createDescription(
        "The most statistically important genes are ordered by adjusted "
        "p-value. Colours are calculated independently for each gene so "
        "that its relative expression pattern can be compared across samples."
    );

    expressionHeatmapSummaryLabel = new QLabel(
        "Run an analysis to generate a heatmap."
    );
    expressionHeatmapSummaryLabel->setAlignment(Qt::AlignCenter);
    expressionHeatmapSummaryLabel->setWordWrap(true);
    expressionHeatmapSummaryLabel->setStyleSheet(
        "font-weight: bold; color: #40566B;"
    );

    QHBoxLayout* legendLayout = new QHBoxLayout;
    legendLayout->addStretch();

    QLabel* lowLabel = new QLabel("  Lower expression  ");
    lowLabel->setAlignment(Qt::AlignCenter);
    lowLabel->setStyleSheet(
        "background-color: #2166AC; color: white; padding: 6px;"
    );

    QLabel* averageLabel = new QLabel("  Gene average  ");
    averageLabel->setAlignment(Qt::AlignCenter);
    averageLabel->setStyleSheet(
        "background-color: #FAFAFA; color: #203040; "
        "border: 1px solid #D7E0E8; padding: 6px;"
    );

    QLabel* highLabel = new QLabel("  Higher expression  ");
    highLabel->setAlignment(Qt::AlignCenter);
    highLabel->setStyleSheet(
        "background-color: #B2182B; color: white; padding: 6px;"
    );

    legendLayout->addWidget(lowLabel);
    legendLayout->addWidget(averageLabel);
    legendLayout->addWidget(highLabel);
    legendLayout->addStretch();

    expressionHeatmapWidget = new ExpressionHeatmapWidget;

    QPushButton* backButton = new QPushButton(
        "Back to Differential Expression Results"
    );
    backButton->setMinimumHeight(44);

    QPushButton* exportButton = new QPushButton(
        "Export Heatmap PNG"
    );
    exportButton->setMinimumHeight(44);

    QHBoxLayout* buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(exportButton);
    buttonLayout->addWidget(backButton);

    layout->addWidget(title);
    layout->addWidget(description);
    layout->addWidget(expressionHeatmapSummaryLabel);
    layout->addLayout(legendLayout);
    layout->addWidget(expressionHeatmapWidget, 1);
    layout->addLayout(buttonLayout);

    connect(
        exportButton,
        &QPushButton::clicked,
        this,
        [this]()
        {
            exportWidgetImage(
                expressionHeatmapWidget,
                "expression_heatmap.png",
                "Export Expression Heatmap"
            );
        }
    );

    connect(
        backButton,
        &QPushButton::clicked,
        this,
        &MainWindow::returnToExpressionResults
    );

    return page;
}

QWidget* MainWindow::createExpressionPCAPage()
{
    QWidget* page = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(page);

    layout->setContentsMargins(30, 20, 30, 20);
    layout->setSpacing(10);

    QLabel* title = createPageTitle(
        "PCA Sample-Clustering Plot"
    );

    QLabel* description = createDescription(
        "Each point represents one biological sample. Samples positioned "
        "near each other have similar overall gene-expression profiles. "
        "Hover over a point to identify the sample."
    );

    pcaSummaryLabel = new QLabel(
        "Run an analysis to calculate PCA."
    );
    pcaSummaryLabel->setAlignment(Qt::AlignCenter);
    pcaSummaryLabel->setWordWrap(true);
    pcaSummaryLabel->setStyleSheet(
        "font-weight: bold; color: #40566B;"
    );

    pcaPlotWidget = new PCAPlotWidget;

    QPushButton* resetZoomButton = new QPushButton(
        "Reset Plot Zoom"
    );
    resetZoomButton->setMinimumHeight(42);

    QPushButton* exportButton = new QPushButton(
        "Export PCA Plot PNG"
    );
    exportButton->setMinimumHeight(42);

    QPushButton* backButton = new QPushButton(
        "Back to Differential Expression Results"
    );
    backButton->setMinimumHeight(44);

    QHBoxLayout* buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(resetZoomButton);
    buttonLayout->addWidget(exportButton);
    buttonLayout->addWidget(backButton);

    layout->addWidget(title);
    layout->addWidget(description);
    layout->addWidget(pcaSummaryLabel);
    layout->addWidget(pcaPlotWidget, 1);
    layout->addLayout(buttonLayout);

    connect(
        resetZoomButton,
        &QPushButton::clicked,
        this,
        [this]()
        {
            pcaPlotWidget->chart()->zoomReset();
        }
    );

    connect(
        exportButton,
        &QPushButton::clicked,
        this,
        [this]()
        {
            exportWidgetImage(
                pcaPlotWidget,
                "pca_sample_plot.png",
                "Export PCA Sample Plot"
            );
        }
    );

    connect(
        backButton,
        &QPushButton::clicked,
        this,
        &MainWindow::returnToExpressionResults
    );

    return page;
}

QWidget* MainWindow::createPhylogeneticQualityPage()
{
    QWidget* page = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(page);

    layout->setContentsMargins(30, 20, 30, 20);
    layout->setSpacing(10);

    QLabel* title = createPageTitle(
        "FASTA Data-Quality Dashboard"
    );

    QLabel* description = createDescription(
        "BioFlow Studio checks sequence counts, identifiers, lengths, "
        "characters, GC content and compatibility with distance methods."
    );

    phylogeneticQualitySummaryLabel = new QLabel(
        "Import FASTA data to generate a quality report."
    );
    phylogeneticQualitySummaryLabel->setAlignment(Qt::AlignCenter);
    phylogeneticQualitySummaryLabel->setWordWrap(true);

    phylogeneticQualityTable = new QTableWidget;
    phylogeneticQualityTable->setColumnCount(4);
    phylogeneticQualityTable->setHorizontalHeaderLabels(
        {"Quality Check", "Status", "Observation", "Recommendation"}
    );
    phylogeneticQualityTable->setEditTriggers(
        QAbstractItemView::NoEditTriggers
    );
    phylogeneticQualityTable->setSelectionBehavior(
        QAbstractItemView::SelectRows
    );
    phylogeneticQualityTable->setWordWrap(true);
    phylogeneticQualityTable->verticalHeader()->setVisible(false);
    phylogeneticQualityTable->horizontalHeader()->setSectionResizeMode(
        QHeaderView::Stretch
    );

    QPushButton* backButton = new QPushButton(
        "Back to Phylogenetic Setup"
    );
    backButton->setMinimumHeight(44);

    layout->addWidget(title);
    layout->addWidget(description);
    layout->addWidget(phylogeneticQualitySummaryLabel);
    layout->addWidget(phylogeneticQualityTable, 1);
    layout->addWidget(backButton);

    connect(
        backButton,
        &QPushButton::clicked,
        this,
        &MainWindow::returnToPhylogeneticSetup
    );

    return page;
}

QWidget* MainWindow::createExpressionQualityPage()
{
    QWidget* page = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(page);

    layout->setContentsMargins(30, 20, 30, 20);
    layout->setSpacing(10);

    QLabel* title = createPageTitle(
        "Expression Data-Quality Dashboard"
    );

    QLabel* description = createDescription(
        "BioFlow Studio checks matrix completeness, identifiers, zeros, "
        "constant genes, sample totals and biological replicate readiness."
    );

    expressionQualitySummaryLabel = new QLabel(
        "Import expression data to generate a quality report."
    );
    expressionQualitySummaryLabel->setAlignment(Qt::AlignCenter);
    expressionQualitySummaryLabel->setWordWrap(true);

    expressionQualityTable = new QTableWidget;
    expressionQualityTable->setColumnCount(4);
    expressionQualityTable->setHorizontalHeaderLabels(
        {"Quality Check", "Status", "Observation", "Recommendation"}
    );
    expressionQualityTable->setEditTriggers(
        QAbstractItemView::NoEditTriggers
    );
    expressionQualityTable->setSelectionBehavior(
        QAbstractItemView::SelectRows
    );
    expressionQualityTable->setWordWrap(true);
    expressionQualityTable->verticalHeader()->setVisible(false);
    expressionQualityTable->horizontalHeader()->setSectionResizeMode(
        QHeaderView::Stretch
    );

    QPushButton* backButton = new QPushButton(
        "Back to Expression Dataset"
    );
    backButton->setMinimumHeight(44);

    layout->addWidget(title);
    layout->addWidget(description);
    layout->addWidget(expressionQualitySummaryLabel);
    layout->addWidget(expressionQualityTable, 1);
    layout->addWidget(backButton);

    connect(
        backButton,
        &QPushButton::clicked,
        this,
        &MainWindow::returnToGeneExpressionSetup
    );

    return page;
}

QWidget* MainWindow::createExpressionEnrichmentPage()
{
    QWidget* page = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(page);

    layout->setContentsMargins(28, 18, 28, 18);
    layout->setSpacing(8);

    QLabel* title = createPageTitle(
        "Functional Enrichment and Pathway Analysis"
    );

    QLabel* description = createDescription(
        "Significant genes are tested for overrepresentation in the "
        "BioFlow curated teaching pathway database using a hypergeometric "
        "test and Benjamini-Hochberg correction."
    );

    enrichmentSummaryLabel = new QLabel(
        "Run differential-expression analysis to identify enriched pathways."
    );
    enrichmentSummaryLabel->setAlignment(Qt::AlignCenter);
    enrichmentSummaryLabel->setWordWrap(true);
    enrichmentSummaryLabel->setStyleSheet(
        "font-weight: bold; color: #40566B;"
    );

    QGridLayout* filterLayout = new QGridLayout;
    filterLayout->setHorizontalSpacing(10);

    QLabel* searchLabel = new QLabel("Search pathway:");
    searchLabel->setStyleSheet("font-weight: bold;");

    pathwaySearchBox = new QLineEdit;
    pathwaySearchBox->setPlaceholderText("Example: cell cycle");
    pathwaySearchBox->setClearButtonEnabled(true);

    QLabel* categoryLabel = new QLabel("Category:");
    categoryLabel->setStyleSheet("font-weight: bold;");

    pathwayCategoryBox = new QComboBox;
    pathwayCategoryBox->addItems(
        {
            "All categories",
            "GO Biological Process",
            "Signalling Pathway",
            "Stress Response",
            "Disease Process"
        }
    );

    QLabel* thresholdLabel = new QLabel("Adjusted p-value <=");
    thresholdLabel->setStyleSheet("font-weight: bold;");

    enrichmentPThresholdBox = new QDoubleSpinBox;
    enrichmentPThresholdBox->setRange(0.000001, 1.0);
    enrichmentPThresholdBox->setDecimals(6);
    enrichmentPThresholdBox->setSingleStep(0.01);
    enrichmentPThresholdBox->setValue(1.0);

    QLabel* maximumLabel = new QLabel("Display:");
    maximumLabel->setStyleSheet("font-weight: bold;");

    maximumPathwaysBox = new QComboBox;
    maximumPathwaysBox->addItem("All pathways", 0);
    maximumPathwaysBox->addItem("Top 5", 5);
    maximumPathwaysBox->addItem("Top 10", 10);
    maximumPathwaysBox->addItem("Top 20", 20);

    QPushButton* resetButton = new QPushButton("Reset Filters");
    resetButton->setMinimumHeight(34);

    filterLayout->addWidget(searchLabel, 0, 0);
    filterLayout->addWidget(pathwaySearchBox, 0, 1);
    filterLayout->addWidget(categoryLabel, 0, 2);
    filterLayout->addWidget(pathwayCategoryBox, 0, 3);
    filterLayout->addWidget(thresholdLabel, 1, 0);
    filterLayout->addWidget(enrichmentPThresholdBox, 1, 1);
    filterLayout->addWidget(maximumLabel, 1, 2);
    filterLayout->addWidget(maximumPathwaysBox, 1, 3);
    filterLayout->addWidget(resetButton, 0, 4, 2, 1);

    enrichmentResultsTable = new QTableWidget;
    enrichmentResultsTable->setColumnCount(8);
    enrichmentResultsTable->setHorizontalHeaderLabels(
        {
            "Pathway ID",
            "Pathway",
            "Category",
            "Overlap",
            "Fold Enrichment",
            "P-value",
            "Adjusted P-value",
            "Contributing Genes"
        }
    );
    enrichmentResultsTable->setEditTriggers(
        QAbstractItemView::NoEditTriggers
    );
    enrichmentResultsTable->setSelectionBehavior(
        QAbstractItemView::SelectRows
    );
    enrichmentResultsTable->setAlternatingRowColors(true);
    enrichmentResultsTable->verticalHeader()->setVisible(false);
    enrichmentResultsTable->horizontalHeader()->setSectionResizeMode(
        QHeaderView::ResizeToContents
    );
    enrichmentResultsTable->horizontalHeader()->setStretchLastSection(true);

    enrichmentBarChart = new EnrichmentBarChartWidget;

    QPushButton* exportButton = new QPushButton(
        "Export Enrichment Results"
    );
    exportButton->setMinimumHeight(42);

    QPushButton* exportChartButton = new QPushButton(
        "Export Pathway Chart PNG"
    );
    exportChartButton->setMinimumHeight(42);

    QPushButton* backButton = new QPushButton(
        "Back to Differential Expression Results"
    );
    backButton->setMinimumHeight(42);

    QHBoxLayout* buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(exportButton);
    buttonLayout->addWidget(exportChartButton);
    buttonLayout->addWidget(backButton);

    layout->addWidget(title);
    layout->addWidget(description);
    layout->addWidget(enrichmentSummaryLabel);
    layout->addLayout(filterLayout);
    layout->addWidget(enrichmentResultsTable, 3);
    layout->addWidget(enrichmentBarChart, 2);
    layout->addLayout(buttonLayout);

    connect(
        pathwaySearchBox,
        &QLineEdit::textChanged,
        this,
        [this]() { applyEnrichmentFilters(); }
    );

    connect(
        pathwayCategoryBox,
        &QComboBox::currentIndexChanged,
        this,
        [this]() { applyEnrichmentFilters(); }
    );

    connect(
        enrichmentPThresholdBox,
        &QDoubleSpinBox::valueChanged,
        this,
        [this]() { applyEnrichmentFilters(); }
    );

    connect(
        maximumPathwaysBox,
        &QComboBox::currentIndexChanged,
        this,
        [this]() { applyEnrichmentFilters(); }
    );

    connect(
        resetButton,
        &QPushButton::clicked,
        this,
        &MainWindow::resetEnrichmentFilters
    );

    connect(
        exportButton,
        &QPushButton::clicked,
        this,
        &MainWindow::exportEnrichmentResults
    );

    connect(
        exportChartButton,
        &QPushButton::clicked,
        this,
        [this]()
        {
            exportWidgetImage(
                enrichmentBarChart,
                "pathway_enrichment_chart.png",
                "Export Pathway-Enrichment Chart"
            );
        }
    );

    connect(
        backButton,
        &QPushButton::clicked,
        this,
        &MainWindow::returnToExpressionResults
    );

    return page;
}

QWidget* MainWindow::createWorkflowBuilderPage()
{
    QWidget* page = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(page);

    layout->setContentsMargins(22, 16, 22, 16);
    layout->setSpacing(8);

    QLabel* title = createPageTitle("Graphical Bioinformatics Workflow Builder");
    QLabel* description = createDescription(
        "Load a biological workflow template, drag nodes to rearrange it, "
        "add custom steps, connect dependencies and validate the workflow."
    );

    workflowTemplateBox = new QComboBox;
    workflowTemplateBox->addItem("Gene Expression Workflow");
    workflowTemplateBox->addItem("Phylogenetic Workflow");

    QPushButton* loadTemplateButton = new QPushButton("Load Template");
    loadTemplateButton->setMinimumHeight(36);

    workflowNodeTypeBox = new QComboBox;
    workflowNodeTypeBox->addItems(
        {"Input Node", "Analysis Node", "Visualization Node", "Output Node"}
    );

    QPushButton* addNodeButton = new QPushButton("Add Step");
    QPushButton* connectButton = new QPushButton("Connect Nodes");
    QPushButton* removeButton = new QPushButton("Remove Selected");
    QPushButton* validateButton = new QPushButton("Validate Workflow");
    QPushButton* exportWorkflowButton = new QPushButton("Export Workflow PNG");

    for (QPushButton* button :
         {addNodeButton, connectButton, removeButton, validateButton,
          exportWorkflowButton})
    {
        button->setMinimumHeight(36);
    }

    QHBoxLayout* templateLayout = new QHBoxLayout;
    templateLayout->addWidget(new QLabel("Template:"));
    templateLayout->addWidget(workflowTemplateBox, 1);
    templateLayout->addWidget(loadTemplateButton);
    templateLayout->addSpacing(15);
    templateLayout->addWidget(new QLabel("New step type:"));
    templateLayout->addWidget(workflowNodeTypeBox);
    templateLayout->addWidget(addNodeButton);

    QHBoxLayout* editLayout = new QHBoxLayout;
    editLayout->addWidget(connectButton);
    editLayout->addWidget(removeButton);
    editLayout->addWidget(validateButton);
    editLayout->addWidget(exportWorkflowButton);
    editLayout->addStretch();

    workflowCanvas = new WorkflowCanvasWidget;

    workflowSelectionLabel = new QLabel(
        "Select a workflow node to inspect its biological purpose."
    );
    workflowSelectionLabel->setWordWrap(true);
    workflowSelectionLabel->setStyleSheet(
        "background-color: white; border: 1px solid #D7E0E8; "
        "border-radius: 6px; padding: 8px; color: #40566B;"
    );

    workflowValidationLabel = new QLabel(
        "Load or edit a workflow, then validate its dependencies."
    );
    workflowValidationLabel->setWordWrap(true);
    workflowValidationLabel->setStyleSheet(
        "font-weight: bold; color: #40566B;"
    );

    openSelectedWorkflowStepButton = new QPushButton(
        "Open Selected Analysis Step"
    );
    openSelectedWorkflowStepButton->setMinimumHeight(42);
    openSelectedWorkflowStepButton->setEnabled(false);

    QPushButton* backButton = new QPushButton("Back to Dashboard");
    backButton->setMinimumHeight(42);

    QHBoxLayout* bottomLayout = new QHBoxLayout;
    bottomLayout->addWidget(openSelectedWorkflowStepButton);
    bottomLayout->addWidget(backButton);

    layout->addWidget(title);
    layout->addWidget(description);
    layout->addLayout(templateLayout);
    layout->addLayout(editLayout);
    layout->addWidget(workflowCanvas, 1);
    layout->addWidget(workflowSelectionLabel);
    layout->addWidget(workflowValidationLabel);
    layout->addLayout(bottomLayout);

    workflowCanvas->setSelectionChangedCallback(
        [this](const WorkflowNode* node)
        {
            if (node == nullptr)
            {
                workflowSelectionLabel->setText(
                    "Select a workflow node to inspect its biological purpose."
                );
                openSelectedWorkflowStepButton->setEnabled(false);
                return;
            }

            workflowSelectionLabel->setText(
                QString("%1 | %2 | Status: %3\n%4")
                    .arg(QString::fromStdString(node->getName()))
                    .arg(QString::fromStdString(node->getCategoryName()))
                    .arg(QString::fromStdString(node->getStateName()))
                    .arg(QString::fromStdString(node->getDescription()))
            );
            openSelectedWorkflowStepButton->setEnabled(true);
        }
    );

    workflowCanvas->setGraphChangedCallback(
        [this]()
        {
            workflowValidationLabel->setText(
                "Workflow modified — validate before using it."
            );
            workflowValidationLabel->setStyleSheet(
                "font-weight: bold; color: #B26A00;"
            );
        }
    );

    workflowCanvas->setMessageCallback(
        [this](const QString& message)
        {
            workflowValidationLabel->setText(message);
            workflowValidationLabel->setStyleSheet(
                "font-weight: bold; color: #245C8A;"
            );
        }
    );

    connect(
        loadTemplateButton,
        &QPushButton::clicked,
        this,
        &MainWindow::loadWorkflowTemplate
    );

    connect(
        addNodeButton,
        &QPushButton::clicked,
        this,
        [this]()
        {
            WorkflowNodeKind kind = WorkflowNodeKind::Processing;

            if (workflowNodeTypeBox->currentIndex() == 0)
            {
                kind = WorkflowNodeKind::Input;
            }
            else if (workflowNodeTypeBox->currentIndex() == 2)
            {
                kind = WorkflowNodeKind::Visualization;
            }
            else if (workflowNodeTypeBox->currentIndex() == 3)
            {
                kind = WorkflowNodeKind::Output;
            }

            workflowCanvas->addNode(kind);
        }
    );

    connect(
        connectButton,
        &QPushButton::clicked,
        this,
        [this]() { workflowCanvas->beginConnectionMode(); }
    );

    connect(
        removeButton,
        &QPushButton::clicked,
        this,
        [this]() { workflowCanvas->removeSelectedNode(); }
    );

    connect(
        validateButton,
        &QPushButton::clicked,
        this,
        [this]()
        {
            std::string validationMessage;
            bool valid = workflowCanvas->getGraph().isValid(
                validationMessage
            );

            workflowValidationLabel->setText(
                QString::fromStdString(validationMessage)
            );
            workflowValidationLabel->setStyleSheet(
                valid
                    ? "font-weight: bold; color: #2D6A4F;"
                    : "font-weight: bold; color: #B02A37;"
            );
        }
    );

    connect(
        exportWorkflowButton,
        &QPushButton::clicked,
        this,
        [this]()
        {
            exportWidgetImage(
                workflowCanvas,
                "bioinformatics_workflow.png",
                "Export Workflow Diagram"
            );
        }
    );

    connect(
        openSelectedWorkflowStepButton,
        &QPushButton::clicked,
        this,
        &MainWindow::openSelectedWorkflowStep
    );

    connect(
        backButton,
        &QPushButton::clicked,
        this,
        &MainWindow::returnToDashboard
    );

    loadWorkflowTemplate();
    return page;
}

QWidget* MainWindow::createProjectHistoryPage()
{
    QWidget* page = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(page);

    layout->setContentsMargins(30, 22, 30, 22);
    layout->setSpacing(10);

    QLabel* title = createPageTitle("Project Analysis History");
    QLabel* description = createDescription(
        "A chronological record of imported datasets and completed "
        "bioinformatics analyses in the current project."
    );

    projectHistorySummaryLabel = new QLabel("No history entries yet.");
    projectHistorySummaryLabel->setAlignment(Qt::AlignCenter);
    projectHistorySummaryLabel->setStyleSheet(
        "font-weight: bold; color: #40566B;"
    );

    projectHistoryTable = new QTableWidget;
    projectHistoryTable->setColumnCount(5);
    projectHistoryTable->setHorizontalHeaderLabels(
        {"Date and Time", "Workspace", "Action", "Status", "Details"}
    );
    projectHistoryTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    projectHistoryTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    projectHistoryTable->setAlternatingRowColors(true);
    projectHistoryTable->verticalHeader()->setVisible(false);
    projectHistoryTable->horizontalHeader()->setSectionResizeMode(
        QHeaderView::ResizeToContents
    );
    projectHistoryTable->horizontalHeader()->setStretchLastSection(true);

    QPushButton* saveButton = new QPushButton("Save Current Project");
    QPushButton* backButton = new QPushButton("Back to Dashboard");
    saveButton->setMinimumHeight(44);
    backButton->setMinimumHeight(44);

    QHBoxLayout* buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(saveButton);
    buttonLayout->addWidget(backButton);

    layout->addWidget(title);
    layout->addWidget(description);
    layout->addWidget(projectHistorySummaryLabel);
    layout->addWidget(projectHistoryTable, 1);
    layout->addLayout(buttonLayout);

    connect(
        saveButton,
        &QPushButton::clicked,
        this,
        &MainWindow::saveProject
    );
    connect(
        backButton,
        &QPushButton::clicked,
        this,
        &MainWindow::returnToDashboard
    );

    return page;
}

void MainWindow::selectPhylogeneticWorkspace()
{
    currentProject.setWorkspaceType(
        WorkspaceType::PhylogeneticAnalysis
    );

    statusLabel->setText(
        "Selected workspace: "
        + QString::fromStdString(
            currentProject.getWorkspaceName()
        )
    );

    pages->setCurrentIndex(
        PhylogeneticSetupPage
    );
}

void MainWindow::selectGeneExpressionWorkspace()
{
    currentProject.setWorkspaceType(
        WorkspaceType::GeneExpressionAnalysis
    );

    statusLabel->setText(
        "Selected workspace: "
        + QString::fromStdString(
            currentProject.getWorkspaceName()
        )
    );

    pages->setCurrentIndex(
        GeneExpressionPage
    );
}

void MainWindow::openWorkflowBuilderPage()
{
    refreshWorkflowNodeStates();
    pages->setCurrentIndex(WorkflowBuilderPage);
}

void MainWindow::openProjectHistoryPage()
{
    refreshProjectHistoryTable();
    pages->setCurrentIndex(ProjectHistoryPage);
}

void MainWindow::loadWorkflowTemplate()
{
    WorkflowTemplateType type = workflowTemplateBox->currentIndex() == 1
        ? WorkflowTemplateType::Phylogenetic
        : WorkflowTemplateType::GeneExpression;

    workflowCanvas->setGraph(WorkflowTemplateFactory::create(type));
    refreshWorkflowNodeStates();

    workflowValidationLabel->setText(
        "Template loaded. Drag nodes to rearrange them or validate the workflow."
    );
    workflowValidationLabel->setStyleSheet(
        "font-weight: bold; color: #2D6A4F;"
    );
}

void MainWindow::refreshWorkflowNodeStates()
{
    if (workflowCanvas == nullptr || workflowTemplateBox == nullptr)
    {
        return;
    }

    bool expressionWorkflow = workflowTemplateBox->currentIndex() == 0;

    for (const auto& nodePointer : workflowCanvas->getGraph().getNodes())
    {
        WorkflowNode* node = nodePointer.get();
        const std::string& name = node->getName();
        WorkflowNodeState state = WorkflowNodeState::Pending;

        if (expressionWorkflow)
        {
            if (name == "Import Expression Data")
            {
                state = expressionDataset
                    ? WorkflowNodeState::Completed
                    : WorkflowNodeState::Ready;
            }
            else if (name == "Data Quality")
            {
                state = expressionDataset
                    ? WorkflowNodeState::Completed
                    : WorkflowNodeState::Blocked;
            }
            else if (name == "Groups and Normalization")
            {
                state = !expressionResults.empty()
                    ? WorkflowNodeState::Completed
                    : expressionDataset
                        ? WorkflowNodeState::Ready
                        : WorkflowNodeState::Blocked;
            }
            else if (name == "Differential Expression")
            {
                state = !expressionResults.empty()
                    ? WorkflowNodeState::Completed
                    : expressionDataset
                        ? WorkflowNodeState::Ready
                        : WorkflowNodeState::Blocked;
            }
            else if (name == "Plots and Exploration")
            {
                state = !expressionResults.empty()
                    ? WorkflowNodeState::Completed
                    : WorkflowNodeState::Blocked;
            }
            else if (name == "Pathway Enrichment")
            {
                state = !enrichmentResults.empty()
                    ? WorkflowNodeState::Completed
                    : !expressionResults.empty()
                        ? WorkflowNodeState::Ready
                        : WorkflowNodeState::Blocked;
            }
            else if (name == "Export and Report")
            {
                state = !expressionResults.empty()
                    ? WorkflowNodeState::Ready
                    : WorkflowNodeState::Blocked;
            }
        }
        else
        {
            if (name == "Import FASTA")
            {
                state = loadedSequences.empty()
                    ? WorkflowNodeState::Ready
                    : WorkflowNodeState::Completed;
            }
            else if (name == "Sequence Quality")
            {
                state = loadedSequences.empty()
                    ? WorkflowNodeState::Blocked
                    : WorkflowNodeState::Completed;
            }
            else if (name == "Distance Matrix")
            {
                state = currentDistanceMatrix.size() > 0
                    || treeDistanceMatrix.size() > 0
                    ? WorkflowNodeState::Completed
                    : loadedSequences.size() >= 2
                        ? WorkflowNodeState::Ready
                        : WorkflowNodeState::Blocked;
            }
            else if (name == "UPGMA Tree"
                     || name == "Tree Visualization")
            {
                state = !currentTree.isEmpty()
                    ? WorkflowNodeState::Completed
                    : loadedSequences.size() >= 2
                        ? WorkflowNodeState::Ready
                        : WorkflowNodeState::Blocked;
            }
            else if (name == "Export and Report")
            {
                state = currentDistanceMatrix.size() > 0
                    || !currentTree.isEmpty()
                    ? WorkflowNodeState::Ready
                    : WorkflowNodeState::Blocked;
            }
        }

        node->setState(state);
    }

    workflowCanvas->update();
}

void MainWindow::openSelectedWorkflowStep()
{
    const WorkflowNode* node = workflowCanvas->getSelectedNode();

    if (node == nullptr)
    {
        return;
    }

    std::string name = node->getName();
    bool expressionWorkflow = workflowTemplateBox->currentIndex() == 0;

    if (expressionWorkflow)
    {
        if (name == "Import Expression Data")
        {
            pages->setCurrentIndex(GeneExpressionPage);
        }
        else if (name == "Data Quality")
        {
            openExpressionQualityPage();
        }
        else if (name == "Groups and Normalization"
                 || name == "Differential Expression")
        {
            openExpressionConfigurationPage();
        }
        else if (name == "Plots and Exploration"
                 || name == "Export and Report")
        {
            if (expressionResults.empty())
            {
                openExpressionConfigurationPage();
            }
            else
            {
                pages->setCurrentIndex(ExpressionResultsPage);
            }
        }
        else if (name == "Pathway Enrichment")
        {
            openExpressionEnrichmentPage();
        }
        else
        {
            QMessageBox::information(
                this,
                "Custom Workflow Step",
                "This custom node demonstrates editable workflow design. "
                "Connect it to a concrete analysis class to make it executable."
            );
        }
    }
    else
    {
        if (name == "Import FASTA")
        {
            pages->setCurrentIndex(PhylogeneticSetupPage);
        }
        else if (name == "Sequence Quality")
        {
            openPhylogeneticQualityPage();
        }
        else if (name == "Distance Matrix")
        {
            openDistanceMatrixPage();
        }
        else if (name == "UPGMA Tree"
                 || name == "Tree Visualization"
                 || name == "Export and Report")
        {
            openPhylogeneticTreePage();
        }
        else
        {
            QMessageBox::information(
                this,
                "Custom Workflow Step",
                "This custom node is not yet connected to an executable "
                "phylogenetic analysis component."
            );
        }
    }
}

void MainWindow::openDistanceMatrixPage()
{
    pages->setCurrentIndex(
        DistanceMatrixPage
    );
}

void MainWindow::openPhylogeneticTreePage()
{
    pages->setCurrentIndex(
        PhylogeneticTreePage
    );
}

void MainWindow::openExpressionConfigurationPage()
{
    if (!expressionDataset)
    {
        QMessageBox::warning(
            this,
            "No Expression Dataset",
            "Please import a valid expression dataset first."
        );
        return;
    }

    populateSampleGroupingTable();
    pages->setCurrentIndex(ExpressionConfigurationPage);
}

void MainWindow::openExpressionVolcanoPage()
{
    if (classifiedExpressionResults.empty())
    {
        QMessageBox::warning(
            this,
            "No Analysis Results",
            "Run differential expression analysis before opening "
            "the volcano plot."
        );
        return;
    }

    volcanoPlotWidget->setResults(
        classifiedExpressionResults,
        adjustedPThresholdBox->value(),
        foldChangeThresholdBox->value()
    );
    pages->setCurrentIndex(ExpressionVolcanoPage);
}

void MainWindow::openExpressionHeatmapPage()
{
    if (!expressionDataset
        || filteredExpressionResults.empty()
        || currentNormalizedExpressionValues.empty())
    {
        QMessageBox::warning(
            this,
            "No Analysis Results",
            "Run differential expression analysis before opening "
            "the expression heatmap."
        );
        return;
    }

    try
    {
        constexpr std::size_t maximumDisplayedGenes = 50;

        expressionHeatmapWidget->setData(
            *expressionDataset,
            currentNormalizedExpressionValues,
            filteredExpressionResults,
            sampleGrouping,
            maximumDisplayedGenes
        );

        std::size_t displayedGenes = std::min(
            maximumDisplayedGenes,
            filteredExpressionResults.size()
        );

        expressionHeatmapSummaryLabel->setText(
            QString(
                "Displaying %1 of %2 genes across %3 samples | "
                "Cell values are row Z-scores"
            )
            .arg(static_cast<qulonglong>(displayedGenes))
            .arg(static_cast<qulonglong>(
                expressionDataset->getGeneCount()
            ))
            .arg(static_cast<qulonglong>(
                expressionDataset->getSampleCount()
            ))
        );
        expressionHeatmapSummaryLabel->setStyleSheet(
            "font-weight: bold; color: #2D6A4F;"
        );

        pages->setCurrentIndex(ExpressionHeatmapPage);
    }
    catch (const std::exception& error)
    {
        QMessageBox::critical(
            this,
            "Heatmap Error",
            QString::fromStdString(error.what())
        );
    }
}

void MainWindow::openExpressionPCAPage()
{
    if (!expressionDataset
        || currentNormalizedExpressionValues.empty())
    {
        QMessageBox::warning(
            this,
            "No Analysis Results",
            "Run differential expression analysis before opening "
            "the PCA plot."
        );
        return;
    }

    try
    {
        PCAAnalyzer analyzer;
        PCAResult result = analyzer.analyze(
            *expressionDataset,
            currentNormalizedExpressionValues
        );

        pcaPlotWidget->setResult(result, sampleGrouping);

        pcaSummaryLabel->setText(
            QString(
                "%1 samples analyzed | PC1 explains %2% | "
                "PC2 explains %3% | Combined: %4%"
            )
            .arg(static_cast<qulonglong>(result.getSampleCount()))
            .arg(result.getPC1ExplainedVariance(), 0, 'f', 1)
            .arg(result.getPC2ExplainedVariance(), 0, 'f', 1)
            .arg(
                result.getPC1ExplainedVariance()
                + result.getPC2ExplainedVariance(),
                0,
                'f',
                1
            )
        );
        pcaSummaryLabel->setStyleSheet(
            "font-weight: bold; color: #2D6A4F;"
        );

        pages->setCurrentIndex(ExpressionPCAPage);
    }
    catch (const std::exception& error)
    {
        QMessageBox::critical(
            this,
            "PCA Error",
            QString::fromStdString(error.what())
        );
    }
}

void MainWindow::openPhylogeneticQualityPage()
{
    if (loadedSequences.empty())
    {
        QMessageBox::warning(
            this,
            "No FASTA Data",
            "Import valid FASTA sequences before opening quality control."
        );
        return;
    }

    std::vector<std::string> sourceFiles;
    sourceFiles.reserve(
        static_cast<std::size_t>(selectedFastaFiles.size())
    );

    for (const QString& filePath : selectedFastaFiles)
    {
        sourceFiles.push_back(filePath.toStdString());
    }

    SequenceQualityAnalyzer analyzer(
        loadedSequences,
        sourceFiles
    );

    QualityReport report = analyzer.analyze();
    populateQualityReportTable(
        phylogeneticQualityTable,
        phylogeneticQualitySummaryLabel,
        report
    );

    recordHistory(
        "Phylogenetic Analysis",
        "Sequence Quality Control",
        "Completed",
        QString("Overall quality status: %1.")
            .arg(QString::fromStdString(
                QualityReport::statusName(report.getOverallStatus())
            ))
    );

    pages->setCurrentIndex(PhylogeneticQualityPage);
}

void MainWindow::openExpressionQualityPage()
{
    if (!expressionDataset)
    {
        QMessageBox::warning(
            this,
            "No Expression Data",
            "Import a valid expression dataset before opening "
            "quality control."
        );
        return;
    }

    ExpressionQualityAnalyzer analyzer(
        *expressionDataset,
        sampleGrouping
    );

    QualityReport report = analyzer.analyze();
    populateQualityReportTable(
        expressionQualityTable,
        expressionQualitySummaryLabel,
        report
    );

    recordHistory(
        "Gene Expression Analysis",
        "Expression Quality Control",
        "Completed",
        QString("Overall quality status: %1.")
            .arg(QString::fromStdString(
                QualityReport::statusName(report.getOverallStatus())
            ))
    );

    pages->setCurrentIndex(ExpressionQualityPage);
}

void MainWindow::openExpressionEnrichmentPage()
{
    if (!expressionDataset || classifiedExpressionResults.empty())
    {
        QMessageBox::warning(
            this,
            "No Differential Expression Results",
            "Run differential-expression analysis before pathway enrichment."
        );
        return;
    }

    runFunctionalEnrichmentAnalysis();
}

void MainWindow::runFunctionalEnrichmentAnalysis()
{
    std::vector<std::string> significantGenes;

    for (const DifferentialExpressionResult& result :
         classifiedExpressionResults)
    {
        if (result.getRegulationStatus()
            != RegulationStatus::NotSignificant)
        {
            significantGenes.push_back(result.getGeneName());
        }
    }

    if (significantGenes.empty())
    {
        QMessageBox::warning(
            this,
            "No Significant Genes",
            "No genes meet the current differential-expression thresholds. "
            "Adjust the p-value or fold-change filters and try again."
        );
        return;
    }

    try
    {
        BuiltInPathwayDatabase database;
        HypergeometricEnrichmentAnalyzer analyzer;

        enrichmentResults = analyzer.analyze(
            significantGenes,
            expressionDataset->getGeneNames(),
            database.getPathways()
        );

        if (enrichmentResults.empty())
        {
            QMessageBox::information(
                this,
                "No Pathway Matches",
                "The significant genes did not overlap the built-in teaching "
                "pathway database. The analysis itself completed correctly."
            );
        }

        resetEnrichmentFilters();
        pages->setCurrentIndex(ExpressionEnrichmentPage);

        recordHistory(
            "Gene Expression Analysis",
            "Functional Enrichment",
            "Completed",
            QString("Tested %1 significant gene(s); returned %2 pathway result(s).")
                .arg(static_cast<qulonglong>(significantGenes.size()))
                .arg(static_cast<qulonglong>(enrichmentResults.size()))
        );
    }
    catch (const std::exception& error)
    {
        QMessageBox::critical(
            this,
            "Enrichment Analysis Error",
            QString::fromStdString(error.what())
        );
    }
}

void MainWindow::applyEnrichmentFilters()
{
    if (enrichmentResultsTable == nullptr)
    {
        return;
    }

    filteredEnrichmentResults.clear();

    QString query = pathwaySearchBox->text().trimmed();
    QString category = pathwayCategoryBox->currentText();
    double maximumAdjustedP = enrichmentPThresholdBox->value();
    std::size_t maximumResults = static_cast<std::size_t>(
        maximumPathwaysBox->currentData().toInt()
    );

    for (const EnrichmentResult& result : enrichmentResults)
    {
        QString pathwayName = QString::fromStdString(
            result.getPathwayName()
        );
        QString pathwayId = QString::fromStdString(
            result.getPathwayId()
        );
        QString geneList = QString::fromStdString(
            result.getOverlappingGeneList()
        );

        bool matchesQuery = query.isEmpty()
            || pathwayName.contains(query, Qt::CaseInsensitive)
            || pathwayId.contains(query, Qt::CaseInsensitive)
            || geneList.contains(query, Qt::CaseInsensitive);

        bool matchesCategory = category == "All categories"
            || QString::fromStdString(result.getCategory()) == category;

        if (matchesQuery
            && matchesCategory
            && result.getAdjustedPValue() <= maximumAdjustedP)
        {
            filteredEnrichmentResults.push_back(result);
        }
    }

    if (maximumResults > 0
        && filteredEnrichmentResults.size() > maximumResults)
    {
        filteredEnrichmentResults.erase(
            filteredEnrichmentResults.begin()
                + static_cast<std::ptrdiff_t>(maximumResults),
            filteredEnrichmentResults.end()
        );
    }

    populateEnrichmentResultsTable();
}

void MainWindow::resetEnrichmentFilters()
{
    pathwaySearchBox->clear();
    pathwayCategoryBox->setCurrentIndex(0);
    enrichmentPThresholdBox->setValue(1.0);
    maximumPathwaysBox->setCurrentIndex(0);

    applyEnrichmentFilters();
}

void MainWindow::populateEnrichmentResultsTable()
{
    enrichmentResultsTable->setSortingEnabled(false);
    enrichmentResultsTable->clearContents();
    enrichmentResultsTable->setRowCount(
        static_cast<int>(filteredEnrichmentResults.size())
    );

    for (std::size_t row = 0;
         row < filteredEnrichmentResults.size();
         ++row)
    {
        const EnrichmentResult& result =
            filteredEnrichmentResults.at(row);
        int tableRow = static_cast<int>(row);

        QTableWidgetItem* pathwayItem = new QTableWidgetItem(
            QString::fromStdString(result.getPathwayName())
        );
        pathwayItem->setTextAlignment(Qt::AlignCenter);

        QTableWidgetItem* adjustedItem = new NumericTableWidgetItem(
            result.getAdjustedPValue(), 7
        );

        if (result.getAdjustedPValue() <= 0.05)
        {
            adjustedItem->setBackground(QColor("#CDEFD8"));
            adjustedItem->setForeground(QColor("#176B35"));
        }

        QTableWidgetItem* overlapItem = new QTableWidgetItem(
            QString("%1 / %2")
                .arg(static_cast<qulonglong>(result.getOverlapCount()))
                .arg(static_cast<qulonglong>(result.getPathwayGeneCount()))
        );
        overlapItem->setTextAlignment(Qt::AlignCenter);

        enrichmentResultsTable->setItem(
            tableRow, 0,
            new QTableWidgetItem(
                QString::fromStdString(result.getPathwayId())
            )
        );
        enrichmentResultsTable->setItem(tableRow, 1, pathwayItem);
        enrichmentResultsTable->setItem(
            tableRow, 2,
            new QTableWidgetItem(
                QString::fromStdString(result.getCategory())
            )
        );
        enrichmentResultsTable->setItem(tableRow, 3, overlapItem);
        enrichmentResultsTable->setItem(
            tableRow, 4,
            new NumericTableWidgetItem(result.getFoldEnrichment(), 6)
        );
        enrichmentResultsTable->setItem(
            tableRow, 5,
            new NumericTableWidgetItem(result.getPValue(), 7)
        );
        enrichmentResultsTable->setItem(tableRow, 6, adjustedItem);
        enrichmentResultsTable->setItem(
            tableRow, 7,
            new QTableWidgetItem(
                QString::fromStdString(result.getOverlappingGeneList())
            )
        );
    }

    enrichmentResultsTable->setSortingEnabled(true);
    enrichmentResultsTable->sortItems(6, Qt::AscendingOrder);
    enrichmentBarChart->setResults(filteredEnrichmentResults, 10);

    std::size_t significantPathways = 0;

    for (const EnrichmentResult& result : enrichmentResults)
    {
        significantPathways += result.getAdjustedPValue() <= 0.05 ? 1 : 0;
    }

    enrichmentSummaryLabel->setText(
        QString(
            "%1 pathway(s) tested | %2 significant | %3 currently shown | "
            "Green adjusted p-values are significant"
        )
        .arg(static_cast<qulonglong>(enrichmentResults.size()))
        .arg(static_cast<qulonglong>(significantPathways))
        .arg(static_cast<qulonglong>(filteredEnrichmentResults.size()))
    );

    enrichmentSummaryLabel->setStyleSheet(
        filteredEnrichmentResults.empty()
            ? "font-weight: bold; color: #B26A00;"
            : "font-weight: bold; color: #2D6A4F;"
    );
}

void MainWindow::returnToDashboard()
{
    pages->setCurrentIndex(
        DashboardPage
    );
}

void MainWindow::returnToPhylogeneticSetup()
{
    pages->setCurrentIndex(
        PhylogeneticSetupPage
    );
}

void MainWindow::returnToGeneExpressionSetup()
{
    pages->setCurrentIndex(GeneExpressionPage);
}

void MainWindow::returnToExpressionResults()
{
    pages->setCurrentIndex(ExpressionResultsPage);
}

void MainWindow::importFastaFiles()
{
    QStringList filePaths =
        QFileDialog::getOpenFileNames(
            this,
            "Select FASTA Files",
            QString(),
            "FASTA Files (*.fasta *.fa *.fna *.faa);;"
            "All Files (*.*)"
        );

    if (filePaths.isEmpty())
    {
        return;
    }

    loadFastaFiles(filePaths, true);
}

void MainWindow::loadFastaFiles(
    const QStringList& filePaths,
    bool shouldRecordHistory
)
{

    FastaParser parser;

    selectedFastaFiles = filePaths;
    loadedSequences.clear();

    currentDistanceMatrix.clear();
    treeDistanceMatrix.clear();
    currentTree.clear();

    phylogeneticFileList->clear();

    distanceMatrixTable->clear();
    distanceMatrixTable->setRowCount(0);
    distanceMatrixTable->setColumnCount(0);

    treeGraphic->clearTree();
    treeOutput->clear();

    openMatrixButton->setEnabled(false);
    openTreeButton->setEnabled(false);
    phylogeneticQualityButton->setEnabled(false);

    try
    {
        for (const QString& filePath :
             selectedFastaFiles)
        {
            QFileInfo fileInformation(
                filePath
            );

            phylogeneticFileList->addItem(
                "FILE: "
                + fileInformation.fileName()
            );

            auto parsedSequences =
                parser.parseFile(
                    filePath.toStdString()
                );

            for (auto& sequence :
                 parsedSequences)
            {
                QString information =
                    QString(
                        "    %1 | %2 | Length: %3"
                    )
                    .arg(
                        QString::fromStdString(
                            sequence->getIdentifier()
                        )
                    )
                    .arg(
                        QString::fromStdString(
                            sequence->getTypeName()
                        )
                    )
                    .arg(
                        static_cast<qulonglong>(
                            sequence->getLength()
                        )
                    );

                phylogeneticFileList->addItem(
                    information
                );

                loadedSequences.push_back(
                    std::move(sequence)
                );
            }
        }

        phylogeneticFileList->insertItem(
            0,
            QString(
                "Successfully loaded %1 "
                "biological sequence(s)"
            ).arg(
                static_cast<qulonglong>(
                    loadedSequences.size()
                )
            )
        );

        bool enoughSequences =
            loadedSequences.size() >= 2;

        openMatrixButton->setEnabled(
            enoughSequences
        );

        openTreeButton->setEnabled(
            enoughSequences
        );

        phylogeneticQualityButton->setEnabled(
            !loadedSequences.empty()
        );

        if (shouldRecordHistory)
        {
            recordHistory(
                "Phylogenetic Analysis",
                "Import FASTA",
                "Completed",
                QString("Loaded %1 sequence(s) from %2 file(s).")
                    .arg(static_cast<qulonglong>(loadedSequences.size()))
                    .arg(selectedFastaFiles.size())
            );
        }
    }
    catch (const std::exception& error)
    {
        loadedSequences.clear();

        phylogeneticFileList->clear();
        phylogeneticFileList->addItem(
            "FASTA import failed."
        );

        QMessageBox::critical(
            this,
            "FASTA Import Error",
            QString::fromStdString(
                error.what()
            )
        );
    }
}

std::unique_ptr<PairwiseAligner>
MainWindow::createAligner(
    const QString& methodName
) const
{
    if (methodName == "Hamming Distance")
    {
        return std::make_unique<
            HammingAligner
        >();
    }

    return std::make_unique<
        NeedlemanWunschAligner
    >();
}

void MainWindow::generateDistanceMatrix()
{
    if (loadedSequences.size() < 2)
    {
        QMessageBox::warning(
            this,
            "Insufficient Sequences",
            "Import at least two compatible sequences."
        );

        return;
    }

    auto aligner = createAligner(
        matrixAlignmentMethodBox->currentText()
    );

    try
    {
        currentDistanceMatrix.calculate(
            loadedSequences,
            *aligner
        );

        populateDistanceMatrixTable(
            currentDistanceMatrix
        );

        matrixStatusLabel->setText(
            QString(
                "%1 | Green = similar, Red = distant"
            ).arg(
                matrixAlignmentMethodBox->currentText()
            )
        );

        matrixStatusLabel->setStyleSheet(
            "font-weight: bold;"
            "color: #2D6A4F;"
        );

        recordHistory(
            "Phylogenetic Analysis",
            "Generate Distance Matrix",
            "Completed",
            QString("%1 sequence(s) analyzed using %2.")
                .arg(static_cast<qulonglong>(loadedSequences.size()))
                .arg(matrixAlignmentMethodBox->currentText())
        );
    }
    catch (const std::exception& error)
    {
        currentDistanceMatrix.clear();

        matrixStatusLabel->setText(
            "Distance matrix generation failed"
        );

        matrixStatusLabel->setStyleSheet(
            "font-weight: bold;"
            "color: #B02A37;"
        );

        QMessageBox::critical(
            this,
            "Distance Matrix Error",
            QString::fromStdString(
                error.what()
            )
        );
    }
}

void MainWindow::populateDistanceMatrixTable(
    const DistanceMatrix& matrix
)
{
    std::size_t matrixSize = matrix.size();
    const auto& matrixValues = matrix.getValues();

    double maximumDistance = 0.0;

    for (const auto& row : matrixValues)
    {
        for (double distance : row)
        {
            maximumDistance = std::max(
                maximumDistance,
                distance
            );
        }
    }

    distanceMatrixTable->clear();

    distanceMatrixTable->setRowCount(
        static_cast<int>(matrixSize)
    );

    distanceMatrixTable->setColumnCount(
        static_cast<int>(matrixSize)
    );

    QStringList labels;

    for (const std::string& label :
         matrix.getLabels())
    {
        labels.append(
            QString::fromStdString(label)
        );
    }

    distanceMatrixTable
        ->setHorizontalHeaderLabels(labels);

    distanceMatrixTable
        ->setVerticalHeaderLabels(labels);

    for (std::size_t row = 0;
         row < matrixSize;
         ++row)
    {
        for (std::size_t column = 0;
             column < matrixSize;
             ++column)
        {
            double distance =
                matrix.getDistance(
                    row,
                    column
                );

            double normalizedDistance =
                maximumDistance > 0.0
                    ? distance / maximumDistance
                    : 0.0;

            normalizedDistance = std::clamp(
                normalizedDistance,
                0.0,
                1.0
            );

            double colorHue =
                (1.0 - normalizedDistance)
                * 0.33;

            QColor cellColor =
                QColor::fromHsvF(
                    colorHue,
                    0.45,
                    1.0
                );

            if (row == column)
            {
                cellColor =
                    QColor("#B7E4C7");
            }

            QTableWidgetItem* item =
                new QTableWidgetItem(
                    QString::number(
                        distance,
                        'f',
                        4
                    )
                );

            item->setTextAlignment(
                Qt::AlignCenter
            );

            item->setBackground(cellColor);
            item->setForeground(
                QColor("#152536")
            );

            item->setToolTip(
                QString(
                    "%1 versus %2\nDistance: %3"
                )
                .arg(
                    labels.at(
                        static_cast<int>(row)
                    )
                )
                .arg(
                    labels.at(
                        static_cast<int>(column)
                    )
                )
                .arg(
                    distance,
                    0,
                    'f',
                    6
                )
            );

            distanceMatrixTable->setItem(
                static_cast<int>(row),
                static_cast<int>(column),
                item
            );
        }
    }

    distanceMatrixTable
        ->horizontalHeader()
        ->setSectionResizeMode(
            QHeaderView::ResizeToContents
        );

    distanceMatrixTable
        ->verticalHeader()
        ->setSectionResizeMode(
            QHeaderView::ResizeToContents
        );
}

void MainWindow::generatePhylogeneticTree()
{
    if (loadedSequences.size() < 2)
    {
        QMessageBox::warning(
            this,
            "Insufficient Sequences",
            "Import at least two compatible sequences."
        );

        return;
    }

    auto aligner = createAligner(
        treeAlignmentMethodBox->currentText()
    );

    try
    {
        treeDistanceMatrix.calculate(
            loadedSequences,
            *aligner
        );

        currentTree.build(
            treeDistanceMatrix
        );

        treeGraphic->setTree(
            &currentTree
        );

        treeOutput->setPlainText(
            QString::fromStdString(
                currentTree.toNewick()
            )
        );

        treeStatusLabel->setText(
            QString(
                "UPGMA tree generated using %1"
            ).arg(
                treeAlignmentMethodBox->currentText()
            )
        );

        treeStatusLabel->setStyleSheet(
            "font-weight: bold;"
            "color: #2D6A4F;"
        );

        recordHistory(
            "Phylogenetic Analysis",
            "Generate UPGMA Tree",
            "Completed",
            QString("Tree built from %1 sequence(s) using %2.")
                .arg(static_cast<qulonglong>(loadedSequences.size()))
                .arg(treeAlignmentMethodBox->currentText())
        );
    }
    catch (const std::exception& error)
    {
        treeDistanceMatrix.clear();
        currentTree.clear();

        treeGraphic->clearTree();
        treeOutput->clear();

        treeStatusLabel->setText(
            "Tree generation failed"
        );

        treeStatusLabel->setStyleSheet(
            "font-weight: bold;"
            "color: #B02A37;"
        );

        QMessageBox::critical(
            this,
            "UPGMA Tree Error",
            QString::fromStdString(
                error.what()
            )
        );
    }
}

void MainWindow::exportMatrixResults()
{
    if (currentDistanceMatrix.size() == 0)
    {
        QMessageBox::warning(
            this,
            "No Matrix",
            "Generate the distance matrix first."
        );

        return;
    }

    QString directory =
        QFileDialog::getExistingDirectory(
            this,
            "Select Matrix Export Folder"
        );

    if (directory.isEmpty())
    {
        return;
    }

    QDir resultsDirectory(directory);

    try
    {
        PhylogeneticExporter::exportCSV(
            currentDistanceMatrix,
            resultsDirectory.filePath(
                "distance_matrix.csv"
            ).toStdString()
        );

        PhylogeneticExporter::exportPhylip(
            currentDistanceMatrix,
            resultsDirectory.filePath(
                "distance_matrix.phy"
            ).toStdString()
        );

        QMessageBox::information(
            this,
            "Export Complete",
            "Exported:\n"
            "- distance_matrix.csv\n"
            "- distance_matrix.phy"
        );
    }
    catch (const std::exception& error)
    {
        QMessageBox::critical(
            this,
            "Export Error",
            QString::fromStdString(
                error.what()
            )
        );
    }
}

void MainWindow::exportTreeResults()
{
    if (currentTree.isEmpty())
    {
        QMessageBox::warning(
            this,
            "No Tree",
            "Generate the UPGMA tree first."
        );

        return;
    }

    QString directory =
        QFileDialog::getExistingDirectory(
            this,
            "Select Tree Export Folder"
        );

    if (directory.isEmpty())
    {
        return;
    }

    QDir resultsDirectory(directory);

    QString newickPath =
        resultsDirectory.filePath(
            "phylogenetic_tree.newick"
        );

    QString imagePath =
        resultsDirectory.filePath(
            "phylogenetic_tree.png"
        );

    try
    {
        PhylogeneticExporter::exportNewick(
            currentTree,
            newickPath.toStdString()
        );

        QPixmap treeImage =
            treeGraphic->grab();

        if (!treeImage.save(
                imagePath,
                "PNG"
            ))
        {
            throw std::runtime_error(
                "Could not export tree image."
            );
        }

        QMessageBox::information(
            this,
            "Export Complete",
            "Exported:\n"
            "- phylogenetic_tree.newick\n"
            "- phylogenetic_tree.png"
        );
    }
    catch (const std::exception& error)
    {
        QMessageBox::critical(
            this,
            "Export Error",
            QString::fromStdString(
                error.what()
            )
        );
    }
}

void MainWindow::importExpressionFile()
{
    QString filePath =
        QFileDialog::getOpenFileName(
            this,
            "Select Gene Expression File",
            QString(),
            "Expression Files (*.csv *.tsv);;"
            "All Files (*.*)"
        );

    if (filePath.isEmpty())
    {
        return;
    }

    loadExpressionFile(filePath, true);
}

void MainWindow::loadExpressionFile(
    const QString& filePath,
    bool shouldRecordHistory
)
{

    ExpressionParser parser;
    expressionQualityButton->setEnabled(false);

    try
    {
        ExpressionDataset parsedDataset =
            parser.parseFile(
                filePath.toStdString()
            );

        expressionDataset =
            std::make_unique<ExpressionDataset>(
                parsedDataset
            );

        sampleGrouping.automaticallyAssign(
            expressionDataset->getSampleNames()
        );
        expressionResults.clear();
        classifiedExpressionResults.clear();
        filteredExpressionResults.clear();
        currentNormalizedExpressionValues.clear();
        enrichmentResults.clear();
        filteredEnrichmentResults.clear();
        configureExpressionButton->setEnabled(true);
        expressionQualityButton->setEnabled(true);

        selectedExpressionFile = filePath;

        QFileInfo fileInformation(filePath);

        expressionFileLabel->setText(
            "Selected file: "
            + fileInformation.fileName()
        );

        expressionFileLabel->setToolTip(
            filePath
        );

        std::size_t geneCount =
            expressionDataset->getGeneCount();

        std::size_t sampleCount =
            expressionDataset->getSampleCount();

        const std::size_t maximumPreviewRows =
            200;

        std::size_t displayedGeneCount =
            std::min(
                geneCount,
                maximumPreviewRows
            );

        expressionPreviewTable->clear();

        expressionPreviewTable->setRowCount(
            static_cast<int>(
                displayedGeneCount
            )
        );

        expressionPreviewTable->setColumnCount(
            static_cast<int>(
                sampleCount
            )
        );

        QStringList sampleLabels;

        for (const std::string& sample :
             expressionDataset->getSampleNames())
        {
            sampleLabels.append(
                QString::fromStdString(sample)
            );
        }

        expressionPreviewTable
            ->setHorizontalHeaderLabels(
                sampleLabels
            );

        QStringList geneLabels;

        for (std::size_t gene = 0;
             gene < displayedGeneCount;
             ++gene)
        {
            geneLabels.append(
                QString::fromStdString(
                    expressionDataset
                        ->getGeneNames()
                        .at(gene)
                )
            );

            for (std::size_t sample = 0;
                 sample < sampleCount;
                 ++sample)
            {
                double value =
                    expressionDataset->getValue(
                        gene,
                        sample
                    );

                QTableWidgetItem* item =
                    new QTableWidgetItem(
                        QString::number(
                            value,
                            'f',
                            3
                        )
                    );

                item->setTextAlignment(
                    Qt::AlignCenter
                );

                expressionPreviewTable->setItem(
                    static_cast<int>(gene),
                    static_cast<int>(sample),
                    item
                );
            }
        }

        expressionPreviewTable
            ->setVerticalHeaderLabels(
                geneLabels
            );

        expressionPreviewTable
            ->horizontalHeader()
            ->setSectionResizeMode(
                QHeaderView::ResizeToContents
            );

        expressionPreviewTable
            ->verticalHeader()
            ->setSectionResizeMode(
                QHeaderView::ResizeToContents
            );

        QString summary =
            QString(
                "Valid dataset | %1 genes | "
                "%2 samples | Status: %3"
            )
            .arg(
                static_cast<qulonglong>(
                    geneCount
                )
            )
            .arg(
                static_cast<qulonglong>(
                    sampleCount
                )
            )
            .arg(
                QString::fromStdString(
                    expressionDataset
                        ->getStatusName()
                )
            );

        if (geneCount > maximumPreviewRows)
        {
            summary += QString(
                " | Previewing first %1 genes"
            ).arg(
                static_cast<qulonglong>(
                    maximumPreviewRows
                )
            );
        }

        expressionSummaryLabel->setText(
            summary
        );

        expressionSummaryLabel->setStyleSheet(
            "font-weight: bold;"
            "color: #2D6A4F;"
        );

        if (shouldRecordHistory)
        {
            recordHistory(
                "Gene Expression Analysis",
                "Import Expression Dataset",
                "Completed",
                QString("Loaded %1 genes across %2 samples.")
                    .arg(static_cast<qulonglong>(geneCount))
                    .arg(static_cast<qulonglong>(sampleCount))
            );
        }
    }
    catch (const std::exception& error)
    {
        expressionDataset.reset();
        sampleGrouping.clear();
        expressionResults.clear();
        classifiedExpressionResults.clear();
        filteredExpressionResults.clear();
        currentNormalizedExpressionValues.clear();
        enrichmentResults.clear();
        filteredEnrichmentResults.clear();
        configureExpressionButton->setEnabled(false);
        expressionQualityButton->setEnabled(false);

        expressionPreviewTable->clear();
        expressionPreviewTable->setRowCount(0);
        expressionPreviewTable->setColumnCount(0);

        expressionFileLabel->setText(
            "Expression file import failed"
        );

        expressionSummaryLabel->setText(
            "Invalid expression dataset"
        );

        expressionSummaryLabel->setStyleSheet(
            "font-weight: bold;"
            "color: #B02A37;"
        );

        QMessageBox::critical(
            this,
            "Expression Import Error",
            QString::fromStdString(
                error.what()
            )
        );
    }
}

void MainWindow::recordHistory(
    const QString& workspace,
    const QString& action,
    const QString& status,
    const QString& details
)
{
    projectSession.addHistory(workspace, action, status, details);

    if (projectHistoryTable != nullptr)
    {
        refreshProjectHistoryTable();
    }
}

void MainWindow::refreshProjectHistoryTable()
{
    if (projectHistoryTable == nullptr)
    {
        return;
    }

    const auto& history = projectSession.history;
    projectHistoryTable->setSortingEnabled(false);
    projectHistoryTable->clearContents();
    projectHistoryTable->setRowCount(static_cast<int>(history.size()));

    for (std::size_t displayRow = 0;
         displayRow < history.size();
         ++displayRow)
    {
        const AnalysisHistoryEntry& entry =
            history.at(history.size() - displayRow - 1);
        int row = static_cast<int>(displayRow);

        projectHistoryTable->setItem(
            row, 0, new QTableWidgetItem(entry.timestamp)
        );
        projectHistoryTable->setItem(
            row, 1, new QTableWidgetItem(entry.workspace)
        );
        projectHistoryTable->setItem(
            row, 2, new QTableWidgetItem(entry.action)
        );

        QTableWidgetItem* statusItem = new QTableWidgetItem(entry.status);
        statusItem->setTextAlignment(Qt::AlignCenter);

        if (entry.status.compare("Completed", Qt::CaseInsensitive) == 0)
        {
            statusItem->setBackground(QColor("#CDEFD8"));
            statusItem->setForeground(QColor("#176B35"));
        }
        else if (entry.status.compare("Failed", Qt::CaseInsensitive) == 0)
        {
            statusItem->setBackground(QColor("#F8D7DA"));
            statusItem->setForeground(QColor("#9C1C1C"));
        }
        else
        {
            statusItem->setBackground(QColor("#FFF1C7"));
            statusItem->setForeground(QColor("#8A5A00"));
        }

        projectHistoryTable->setItem(row, 3, statusItem);
        projectHistoryTable->setItem(
            row, 4, new QTableWidgetItem(entry.details)
        );
    }

    projectHistorySummaryLabel->setText(
        history.empty()
            ? "No history entries yet."
            : QString("%1 recorded project event(s) | Newest first")
                .arg(static_cast<qulonglong>(history.size()))
    );
}

void MainWindow::saveProject()
{
    QString filePath = currentProjectFile;

    if (filePath.isEmpty())
    {
        filePath = QFileDialog::getSaveFileName(
            this,
            "Save BioFlow Project",
            "BioFlow_Project.bioflow",
            "BioFlow Projects (*.bioflow)"
        );
    }

    if (filePath.isEmpty())
    {
        return;
    }

    if (!filePath.endsWith(".bioflow", Qt::CaseInsensitive))
    {
        filePath += ".bioflow";
    }

    projectSession.projectName = QFileInfo(filePath).completeBaseName();
    projectSession.savedAt = QDateTime::currentDateTime().toString(Qt::ISODate);
    projectSession.fastaFilePaths = selectedFastaFiles;
    projectSession.expressionFilePath = selectedExpressionFile;
    projectSession.matrixMethodIndex = matrixAlignmentMethodBox->currentIndex();
    projectSession.treeMethodIndex = treeAlignmentMethodBox->currentIndex();
    projectSession.normalizationMethodIndex =
        normalizationMethodBox->currentIndex();
    projectSession.adjustedPValueThreshold = adjustedPThresholdBox->value();
    projectSession.foldChangeThreshold = foldChangeThresholdBox->value();
    projectSession.enrichmentPValueThreshold =
        enrichmentPThresholdBox->value();
    projectSession.workflowTemplateIndex = workflowTemplateBox->currentIndex();
    projectSession.sampleGroups.clear();

    if (!sampleGroupBoxes.empty())
    {
        for (const QComboBox* groupBox : sampleGroupBoxes)
        {
            projectSession.sampleGroups.push_back(groupBox->currentIndex());
        }
    }
    else
    {
        for (SampleGroup group : sampleGrouping.getGroups())
        {
            projectSession.sampleGroups.push_back(static_cast<int>(group));
        }
    }

    try
    {
        projectSession.addHistory(
            "Project",
            "Save Project",
            "Completed",
            "Project saved to " + filePath
        );
        ProjectSerializer::save(filePath, projectSession);

        currentProjectFile = filePath;
        setWindowTitle(
            "BioFlow Studio - " + projectSession.projectName
        );
        refreshProjectHistoryTable();
        statusLabel->setText(
            "Project saved: " + QFileInfo(filePath).fileName()
        );

        QMessageBox::information(
            this,
            "Project Saved",
            "The project, analysis settings and history were saved.\n\n"
            "Project file: " + filePath
        );
    }
    catch (const std::exception& error)
    {
        if (!projectSession.history.empty()
            && projectSession.history.back().action == "Save Project"
            && projectSession.history.back().status == "Completed")
        {
            projectSession.history.pop_back();
        }

        recordHistory(
            "Project",
            "Save Project",
            "Failed",
            QString::fromStdString(error.what())
        );
        QMessageBox::critical(
            this,
            "Project Save Error",
            QString::fromStdString(error.what())
        );
    }
}

void MainWindow::loadProject()
{
    QString filePath = QFileDialog::getOpenFileName(
        this,
        "Open BioFlow Project",
        QString(),
        "BioFlow Projects (*.bioflow);;JSON Files (*.json);;All Files (*.*)"
    );

    if (filePath.isEmpty())
    {
        return;
    }

    try
    {
        ProjectSession loadedSession = ProjectSerializer::load(filePath);
        projectSession = loadedSession;
        currentProjectFile = filePath;

        QStringList existingFastaFiles;
        QStringList missingFiles;

        for (const QString& fastaPath : projectSession.fastaFilePaths)
        {
            if (QFileInfo::exists(fastaPath))
            {
                existingFastaFiles.append(fastaPath);
            }
            else
            {
                missingFiles.append(fastaPath);
            }
        }

        if (!existingFastaFiles.isEmpty())
        {
            loadFastaFiles(existingFastaFiles, false);
        }

        if (!projectSession.expressionFilePath.isEmpty())
        {
            if (QFileInfo::exists(projectSession.expressionFilePath))
            {
                loadExpressionFile(projectSession.expressionFilePath, false);
            }
            else
            {
                missingFiles.append(projectSession.expressionFilePath);
            }
        }

        auto restoreComboIndex = [](QComboBox* box, int index)
        {
            if (box != nullptr && index >= 0 && index < box->count())
            {
                box->setCurrentIndex(index);
            }
        };

        restoreComboIndex(
            matrixAlignmentMethodBox,
            projectSession.matrixMethodIndex
        );
        restoreComboIndex(
            treeAlignmentMethodBox,
            projectSession.treeMethodIndex
        );
        restoreComboIndex(
            normalizationMethodBox,
            projectSession.normalizationMethodIndex
        );
        restoreComboIndex(
            workflowTemplateBox,
            projectSession.workflowTemplateIndex
        );

        adjustedPThresholdBox->setValue(
            projectSession.adjustedPValueThreshold
        );
        foldChangeThresholdBox->setValue(
            projectSession.foldChangeThreshold
        );
        enrichmentPThresholdBox->setValue(
            projectSession.enrichmentPValueThreshold
        );

        if (expressionDataset
            && projectSession.sampleGroups.size()
                == expressionDataset->getSampleCount())
        {
            for (std::size_t index = 0;
                 index < projectSession.sampleGroups.size();
                 ++index)
            {
                int groupValue = projectSession.sampleGroups.at(index);
                groupValue = std::clamp(groupValue, 0, 2);
                sampleGrouping.setGroup(
                    index,
                    static_cast<SampleGroup>(groupValue)
                );
            }

            populateSampleGroupingTable();
        }

        loadWorkflowTemplate();
        recordHistory(
            "Project",
            "Open Project",
            "Completed",
            "Loaded " + QFileInfo(filePath).fileName()
        );

        setWindowTitle(
            "BioFlow Studio - " + projectSession.projectName
        );
        statusLabel->setText(
            "Project loaded: " + projectSession.projectName
        );
        pages->setCurrentIndex(DashboardPage);

        QString message =
            "Project data references, settings and analysis history were "
            "restored. Re-run analyses to regenerate calculated results.";

        if (!missingFiles.isEmpty())
        {
            message += "\n\nThe following source file(s) could not be found:\n- "
                + missingFiles.join("\n- ");
        }

        QMessageBox::information(
            this,
            "Project Loaded",
            message
        );
    }
    catch (const std::exception& error)
    {
        QMessageBox::critical(
            this,
            "Project Load Error",
            QString::fromStdString(error.what())
        );
    }
}

void MainWindow::populateQualityReportTable(
    QTableWidget* table,
    QLabel* summaryLabel,
    const QualityReport& report
)
{
    table->clearContents();
    table->setRowCount(
        static_cast<int>(report.getChecks().size())
    );

    for (std::size_t row = 0;
         row < report.getChecks().size();
         ++row)
    {
        const QualityCheck& check = report.getChecks().at(row);

        QTableWidgetItem* nameItem = new QTableWidgetItem(
            QString::fromStdString(check.name)
        );
        QTableWidgetItem* statusItem = new QTableWidgetItem(
            QString::fromStdString(
                QualityReport::statusName(check.status)
            )
        );
        QTableWidgetItem* observationItem = new QTableWidgetItem(
            QString::fromStdString(check.observation)
        );
        QTableWidgetItem* recommendationItem = new QTableWidgetItem(
            QString::fromStdString(check.recommendation)
        );

        statusItem->setTextAlignment(Qt::AlignCenter);

        QColor background;
        QColor foreground;

        if (check.status == QualityStatus::Pass)
        {
            background = QColor("#CDEFD8");
            foreground = QColor("#176B35");
        }
        else if (check.status == QualityStatus::Warning)
        {
            background = QColor("#FFF0BF");
            foreground = QColor("#8A5A00");
        }
        else
        {
            background = QColor("#FFD6D6");
            foreground = QColor("#9C1C1C");
        }

        statusItem->setBackground(background);
        statusItem->setForeground(foreground);

        int tableRow = static_cast<int>(row);
        table->setItem(tableRow, 0, nameItem);
        table->setItem(tableRow, 1, statusItem);
        table->setItem(tableRow, 2, observationItem);
        table->setItem(tableRow, 3, recommendationItem);
    }

    table->resizeRowsToContents();

    summaryLabel->setText(
        QString(
            "Overall status: %1 | %2 passed | %3 warning(s) | %4 failed"
        )
        .arg(QString::fromStdString(report.getOverallStatusName()))
        .arg(static_cast<qulonglong>(report.getPassCount()))
        .arg(static_cast<qulonglong>(report.getWarningCount()))
        .arg(static_cast<qulonglong>(report.getFailCount()))
    );

    if (report.getOverallStatus() == QualityStatus::Pass)
    {
        summaryLabel->setStyleSheet(
            "font-weight: bold; color: #176B35;"
        );
    }
    else if (report.getOverallStatus() == QualityStatus::Warning)
    {
        summaryLabel->setStyleSheet(
            "font-weight: bold; color: #8A5A00;"
        );
    }
    else
    {
        summaryLabel->setStyleSheet(
            "font-weight: bold; color: #9C1C1C;"
        );
    }
}

void MainWindow::populateSampleGroupingTable()
{
    if (!expressionDataset)
    {
        return;
    }

    const auto& sampleNames = expressionDataset->getSampleNames();

    if (sampleGrouping.getGroups().size() != sampleNames.size())
    {
        sampleGrouping.automaticallyAssign(sampleNames);
    }

    sampleGroupingTable->clearContents();
    sampleGroupingTable->setRowCount(
        static_cast<int>(sampleNames.size())
    );
    sampleGroupBoxes.clear();
    sampleGroupBoxes.reserve(sampleNames.size());

    for (std::size_t index = 0; index < sampleNames.size(); ++index)
    {
        QTableWidgetItem* sampleItem = new QTableWidgetItem(
            QString::fromStdString(sampleNames.at(index))
        );
        sampleItem->setTextAlignment(Qt::AlignCenter);

        QComboBox* groupBox = new QComboBox;
        groupBox->addItems(
            {"Unassigned", "Control", "Treatment"}
        );

        SampleGroup group = sampleGrouping.getGroup(index);

        if (group == SampleGroup::Control)
        {
            groupBox->setCurrentIndex(1);
        }
        else if (group == SampleGroup::Treatment)
        {
            groupBox->setCurrentIndex(2);
        }
        else
        {
            groupBox->setCurrentIndex(0);
        }

        sampleGroupingTable->setItem(
            static_cast<int>(index),
            0,
            sampleItem
        );
        sampleGroupingTable->setCellWidget(
            static_cast<int>(index),
            1,
            groupBox
        );

        sampleGroupBoxes.push_back(groupBox);

        connect(
            groupBox,
            &QComboBox::currentIndexChanged,
            this,
            [this]()
            {
                std::size_t controlCount = 0;
                std::size_t treatmentCount = 0;

                for (const QComboBox* box : sampleGroupBoxes)
                {
                    if (box->currentIndex() == 1)
                    {
                        ++controlCount;
                    }
                    else if (box->currentIndex() == 2)
                    {
                        ++treatmentCount;
                    }
                }

                groupingStatusLabel->setText(
                    QString("Control: %1 sample(s) | Treatment: %2 sample(s)")
                        .arg(static_cast<qulonglong>(controlCount))
                        .arg(static_cast<qulonglong>(treatmentCount))
                );

                bool valid = controlCount >= 2 && treatmentCount >= 2;
                groupingStatusLabel->setStyleSheet(
                    valid
                        ? "font-weight: bold; color: #2D6A4F;"
                        : "font-weight: bold; color: #B26A00;"
                );
            }
        );
    }

    std::size_t controlCount = 0;
    std::size_t treatmentCount = 0;

    for (const QComboBox* box : sampleGroupBoxes)
    {
        controlCount += box->currentIndex() == 1 ? 1 : 0;
        treatmentCount += box->currentIndex() == 2 ? 1 : 0;
    }

    groupingStatusLabel->setText(
        QString("Control: %1 sample(s) | Treatment: %2 sample(s)")
            .arg(static_cast<qulonglong>(controlCount))
            .arg(static_cast<qulonglong>(treatmentCount))
    );

    groupingStatusLabel->setStyleSheet(
        controlCount >= 2 && treatmentCount >= 2
            ? "font-weight: bold; color: #2D6A4F;"
            : "font-weight: bold; color: #B26A00;"
    );
}

void MainWindow::runDifferentialExpressionAnalysis()
{
    if (!expressionDataset)
    {
        QMessageBox::warning(
            this,
            "No Expression Dataset",
            "Please import a valid expression dataset first."
        );
        return;
    }

    sampleGrouping.automaticallyAssign(
        expressionDataset->getSampleNames()
    );

    for (std::size_t index = 0;
         index < sampleGroupBoxes.size();
         ++index)
    {
        SampleGroup group = SampleGroup::Unassigned;

        if (sampleGroupBoxes.at(index)->currentIndex() == 1)
        {
            group = SampleGroup::Control;
        }
        else if (sampleGroupBoxes.at(index)->currentIndex() == 2)
        {
            group = SampleGroup::Treatment;
        }

        sampleGrouping.setGroup(index, group);
    }

    if (!sampleGrouping.isValid())
    {
        QMessageBox::warning(
            this,
            "Invalid Sample Groups",
            "Assign at least two samples to Control and at least two "
            "samples to Treatment."
        );
        return;
    }

    try
    {
        std::unique_ptr<NormalizationStrategy> normalizer;

        if (normalizationMethodBox->currentIndex() == 0)
        {
            normalizer = std::make_unique<RawNormalization>();
        }
        else if (normalizationMethodBox->currentIndex() == 1)
        {
            normalizer = std::make_unique<Log2Normalization>();
        }
        else
        {
            normalizer = std::make_unique<ZScoreNormalization>();
        }

        currentNormalizedExpressionValues =
            normalizer->normalize(*expressionDataset);

        WelchTTestAnalyzer analyzer;
        expressionResults = analyzer.analyze(
            *expressionDataset,
            currentNormalizedExpressionValues,
            sampleGrouping
        );

        enrichmentResults.clear();
        filteredEnrichmentResults.clear();

        resetExpressionFilters();
        pages->setCurrentIndex(ExpressionResultsPage);

        recordHistory(
            "Gene Expression Analysis",
            "Differential Expression",
            "Completed",
            QString("Analyzed %1 gene(s) using %2 normalization.")
                .arg(static_cast<qulonglong>(expressionResults.size()))
                .arg(normalizationMethodBox->currentText())
        );
    }
    catch (const std::exception& error)
    {
        QMessageBox::critical(
            this,
            "Differential Expression Error",
            QString::fromStdString(error.what())
        );
    }
}

ExpressionFilterSettings
MainWindow::getCurrentExpressionFilterSettings() const
{
    ExpressionFilterSettings settings;

    settings.geneQuery = geneSearchBox->text().toStdString();
    settings.adjustedPValueThreshold = adjustedPThresholdBox->value();
    settings.minimumAbsoluteLog2FoldChange =
        foldChangeThresholdBox->value();
    settings.maximumResults = static_cast<std::size_t>(
        maximumResultsBox->currentData().toInt()
    );

    switch (regulationFilterBox->currentIndex())
    {
        case 1:
            settings.regulationFilter = RegulationFilter::Upregulated;
            break;

        case 2:
            settings.regulationFilter = RegulationFilter::Downregulated;
            break;

        case 3:
            settings.regulationFilter = RegulationFilter::NotSignificant;
            break;

        default:
            settings.regulationFilter = RegulationFilter::All;
            break;
    }

    return settings;
}

void MainWindow::applyExpressionFilters()
{
    if (expressionResults.empty())
    {
        return;
    }

    ExpressionFilterSettings settings =
        getCurrentExpressionFilterSettings();

    ExpressionFilterEngine filterEngine;
    classifiedExpressionResults = filterEngine.classify(
        expressionResults,
        settings
    );
    filteredExpressionResults = filterEngine.filter(
        classifiedExpressionResults,
        settings
    );

    populateExpressionResultsTable();

    expressionFilterSummaryLabel->setText(
        QString(
            "Showing %1 of %2 genes | Adjusted p <= %3 | "
            "Minimum |log2 FC| = %4"
        )
        .arg(static_cast<qulonglong>(filteredExpressionResults.size()))
        .arg(static_cast<qulonglong>(classifiedExpressionResults.size()))
        .arg(settings.adjustedPValueThreshold, 0, 'g', 5)
        .arg(settings.minimumAbsoluteLog2FoldChange, 0, 'f', 2)
    );

    expressionFilterSummaryLabel->setStyleSheet(
        filteredExpressionResults.empty()
            ? "font-weight: bold; color: #B26A00;"
            : "font-weight: bold; color: #2D6A4F;"
    );
}

void MainWindow::resetExpressionFilters()
{
    geneSearchBox->clear();
    regulationFilterBox->setCurrentIndex(0);
    adjustedPThresholdBox->setValue(0.05);
    foldChangeThresholdBox->setValue(1.0);
    maximumResultsBox->setCurrentIndex(0);

    applyExpressionFilters();
}

void MainWindow::populateExpressionResultsTable()
{
    expressionResultsTable->setSortingEnabled(false);
    expressionResultsTable->clearContents();
    expressionResultsTable->setRowCount(
        static_cast<int>(filteredExpressionResults.size())
    );

    std::size_t upregulatedCount = 0;
    std::size_t downregulatedCount = 0;

    for (const DifferentialExpressionResult& result :
         classifiedExpressionResults)
    {
        if (result.getRegulationStatus() == RegulationStatus::Upregulated)
        {
            ++upregulatedCount;
        }
        else if (result.getRegulationStatus()
                 == RegulationStatus::Downregulated)
        {
            ++downregulatedCount;
        }
    }

    for (std::size_t row = 0;
         row < filteredExpressionResults.size();
         ++row)
    {
        const DifferentialExpressionResult& result =
            filteredExpressionResults.at(row);

        QTableWidgetItem* geneItem = new QTableWidgetItem(
            QString::fromStdString(result.getGeneName())
        );
        geneItem->setTextAlignment(Qt::AlignCenter);

        QTableWidgetItem* regulationItem = new QTableWidgetItem(
            QString::fromStdString(result.getRegulationName())
        );
        regulationItem->setTextAlignment(Qt::AlignCenter);

        if (result.getRegulationStatus() == RegulationStatus::Upregulated)
        {
            regulationItem->setBackground(QColor("#CDEFD8"));
            regulationItem->setForeground(QColor("#176B35"));
        }
        else if (result.getRegulationStatus() == RegulationStatus::Downregulated)
        {
            regulationItem->setBackground(QColor("#FFD6D6"));
            regulationItem->setForeground(QColor("#9C1C1C"));
        }
        else
        {
            regulationItem->setBackground(QColor("#E8EDF2"));
            regulationItem->setForeground(QColor("#40566B"));
        }

        int tableRow = static_cast<int>(row);
        expressionResultsTable->setItem(tableRow, 0, geneItem);
        expressionResultsTable->setItem(
            tableRow, 1,
            new NumericTableWidgetItem(result.getControlMean(), 7)
        );
        expressionResultsTable->setItem(
            tableRow, 2,
            new NumericTableWidgetItem(result.getTreatmentMean(), 7)
        );
        expressionResultsTable->setItem(
            tableRow, 3,
            new NumericTableWidgetItem(result.getLog2FoldChange(), 7)
        );
        expressionResultsTable->setItem(
            tableRow, 4,
            new NumericTableWidgetItem(result.getPValue(), 7)
        );
        expressionResultsTable->setItem(
            tableRow, 5,
            new NumericTableWidgetItem(result.getAdjustedPValue(), 7)
        );
        expressionResultsTable->setItem(tableRow, 6, regulationItem);
    }

    std::size_t significantCount =
        upregulatedCount + downregulatedCount;

    expressionResultsSummaryLabel->setText(
        QString(
            "%1 genes analyzed | %2 significant | "
            "%3 upregulated | %4 downregulated | Normalization: %5"
        )
        .arg(static_cast<qulonglong>(classifiedExpressionResults.size()))
        .arg(static_cast<qulonglong>(significantCount))
        .arg(static_cast<qulonglong>(upregulatedCount))
        .arg(static_cast<qulonglong>(downregulatedCount))
        .arg(normalizationMethodBox->currentText())
    );
    expressionResultsSummaryLabel->setStyleSheet(
        "font-weight: bold; color: #2D6A4F;"
    );

    expressionResultsTable->setSortingEnabled(true);
    expressionResultsTable->sortItems(5, Qt::AscendingOrder);
}

void MainWindow::exportExpressionTables()
{
    if (!expressionDataset
        || filteredExpressionResults.empty()
        || currentNormalizedExpressionValues.empty())
    {
        QMessageBox::warning(
            this,
            "Nothing to Export",
            "Run differential expression analysis before exporting results."
        );
        return;
    }

    QString directoryPath = QFileDialog::getExistingDirectory(
        this,
        "Select Analysis Export Folder",
        QString(),
        QFileDialog::ShowDirsOnly
    );

    if (directoryPath.isEmpty())
    {
        return;
    }

    try
    {
        QDir exportDirectory(directoryPath);
        GeneExpressionExporter exporter;

        exporter.exportResultsCSV(
            exportDirectory
                .filePath("differential_expression_results.csv")
                .toStdString(),
            filteredExpressionResults
        );

        exporter.exportNormalizedMatrixCSV(
            exportDirectory
                .filePath("normalized_expression_matrix.csv")
                .toStdString(),
            *expressionDataset,
            currentNormalizedExpressionValues,
            sampleGrouping
        );

        exporter.exportAnalysisSummary(
            exportDirectory
                .filePath("analysis_summary.txt")
                .toStdString(),
            *expressionDataset,
            classifiedExpressionResults,
            sampleGrouping,
            normalizationMethodBox->currentText().toStdString(),
            adjustedPThresholdBox->value(),
            foldChangeThresholdBox->value()
        );

        QMessageBox::information(
            this,
            "Export Completed",
            "Three files were exported successfully:\n\n"
            "differential_expression_results.csv\n"
            "normalized_expression_matrix.csv\n"
            "analysis_summary.txt\n\n"
            "Folder: " + directoryPath
        );
    }
    catch (const std::exception& error)
    {
        QMessageBox::critical(
            this,
            "Expression Export Error",
            QString::fromStdString(error.what())
        );
    }
}

void MainWindow::exportEnrichmentResults()
{
    if (filteredEnrichmentResults.empty())
    {
        QMessageBox::warning(
            this,
            "Nothing to Export",
            "No enrichment results match the current filters."
        );
        return;
    }

    QString filePath = QFileDialog::getSaveFileName(
        this,
        "Export Functional Enrichment Results",
        "functional_enrichment_results.csv",
        "CSV Files (*.csv)"
    );

    if (filePath.isEmpty())
    {
        return;
    }

    if (!filePath.endsWith(".csv", Qt::CaseInsensitive))
    {
        filePath += ".csv";
    }

    try
    {
        std::ofstream output(filePath.toStdString());

        if (!output.is_open())
        {
            throw std::runtime_error(
                "Could not create the enrichment CSV file."
            );
        }

        auto quoteCSV = [](const std::string& value)
        {
            std::string escaped;
            escaped.reserve(value.size() + 2);
            escaped.push_back('"');

            for (char character : value)
            {
                if (character == '"')
                {
                    escaped.push_back('"');
                }

                escaped.push_back(character);
            }

            escaped.push_back('"');
            return escaped;
        };

        output
            << "Pathway ID,Pathway,Category,Overlap Count,"
            << "Pathway Genes in Background,Fold Enrichment,P-value,"
            << "Adjusted P-value,Contributing Genes\n";

        output << std::setprecision(10);

        for (const EnrichmentResult& result : filteredEnrichmentResults)
        {
            output
                << quoteCSV(result.getPathwayId()) << ','
                << quoteCSV(result.getPathwayName()) << ','
                << quoteCSV(result.getCategory()) << ','
                << result.getOverlapCount() << ','
                << result.getPathwayGeneCount() << ','
                << result.getFoldEnrichment() << ','
                << result.getPValue() << ','
                << result.getAdjustedPValue() << ','
                << quoteCSV(result.getOverlappingGeneList())
                << '\n';
        }

        QMessageBox::information(
            this,
            "Export Completed",
            "Functional-enrichment results were exported successfully.\n\n"
            "File: " + filePath
        );
    }
    catch (const std::exception& error)
    {
        QMessageBox::critical(
            this,
            "Enrichment Export Error",
            QString::fromStdString(error.what())
        );
    }
}

bool MainWindow::saveReportImage(
    QWidget* widget,
    const QString& filePath,
    int width,
    int height
)
{
    if (widget == nullptr)
    {
        return false;
    }

    QSize originalSize = widget->size();
    widget->resize(width, height);
    widget->ensurePolished();

    QChartView* chartView = qobject_cast<QChartView*>(widget);

    if (chartView != nullptr && chartView->chart() != nullptr)
    {
        chartView->chart()->setAnimationOptions(QChart::NoAnimation);
        chartView->chart()->resize(width, height);
        chartView->chart()->update();
        chartView->viewport()->update();
    }

    QApplication::processEvents();

    QPixmap image(width, height);
    image.fill(Qt::white);

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    widget->render(&painter);
    painter.end();

    bool saved = image.save(filePath, "PNG");

    widget->resize(originalSize);
    return saved;
}

void MainWindow::generateExpressionHtmlReport()
{
    if (!expressionDataset
        || classifiedExpressionResults.empty()
        || currentNormalizedExpressionValues.empty())
    {
        QMessageBox::warning(
            this,
            "No Expression Analysis",
            "Run differential-expression analysis before generating a report."
        );
        return;
    }

    QString parentPath = QFileDialog::getExistingDirectory(
        this,
        "Select Folder for Expression Analysis Report",
        QString(),
        QFileDialog::ShowDirsOnly
    );

    if (parentPath.isEmpty())
    {
        return;
    }

    try
    {
        QString timeStamp = QDateTime::currentDateTime().toString(
            "yyyyMMdd_HHmmss"
        );
        QString folderName = "BioFlow_Expression_Report_" + timeStamp;
        QDir parentDirectory(parentPath);

        if (!parentDirectory.mkpath(folderName))
        {
            throw std::runtime_error("Could not create the report folder.");
        }

        QDir reportDirectory(parentDirectory.filePath(folderName));
        AnalysisReport report;
        report.title = "BioFlow Studio Gene-Expression Analysis Report";
        report.subtitle = "Differential expression, quality control, "
            "visualization and pathway interpretation";
        report.generatedAt = QDateTime::currentDateTime()
            .toString("dddd, dd MMMM yyyy — hh:mm AP")
            .toStdString();
        report.overview =
            "This automatically generated report summarizes the imported "
            "expression matrix, statistical analysis, significant genes, "
            "quality checks and graphical results.";

        std::size_t significantCount = 0;
        std::size_t upregulatedCount = 0;
        std::size_t downregulatedCount = 0;

        for (const DifferentialExpressionResult& result :
             classifiedExpressionResults)
        {
            if (result.getRegulationStatus()
                == RegulationStatus::Upregulated)
            {
                ++significantCount;
                ++upregulatedCount;
            }
            else if (result.getRegulationStatus()
                     == RegulationStatus::Downregulated)
            {
                ++significantCount;
                ++downregulatedCount;
            }
        }

        report.metadata = {
            {"Dataset", QFileInfo(selectedExpressionFile)
                .fileName().toStdString()},
            {"Genes", std::to_string(expressionDataset->getGeneCount())},
            {"Samples", std::to_string(expressionDataset->getSampleCount())},
            {"Normalization", normalizationMethodBox
                ->currentText().toStdString()},
            {"Statistical Test", "Welch two-sample t-test"},
            {"Multiple Testing", "Benjamini-Hochberg FDR correction"},
            {"Significant Genes", std::to_string(significantCount)},
            {"Up / Down", std::to_string(upregulatedCount)
                + " / " + std::to_string(downregulatedCount)},
            {"Control / Treatment", std::to_string(
                sampleGrouping.getControlCount()) + " / "
                + std::to_string(sampleGrouping.getTreatmentCount())}
        };

        ExpressionQualityAnalyzer qualityAnalyzer(
            *expressionDataset,
            sampleGrouping
        );
        QualityReport qualityReport = qualityAnalyzer.analyze();
        ReportTable qualityTable;
        qualityTable.title = "Expression Data-Quality Assessment";
        qualityTable.headers = {
            "Check", "Status", "Observation", "Recommendation"
        };

        for (const QualityCheck& check : qualityReport.getChecks())
        {
            qualityTable.rows.push_back({
                check.name,
                QualityReport::statusName(check.status),
                check.observation,
                check.recommendation
            });
        }

        report.tables.push_back(qualityTable);

        ReportTable resultTable;
        resultTable.title = "Differential-Expression Results";
        resultTable.headers = {
            "Gene", "Control Mean", "Treatment Mean", "Log2 Fold Change",
            "P-value", "Adjusted P-value", "Regulation"
        };

        std::size_t resultLimit = std::min<std::size_t>(
            100,
            classifiedExpressionResults.size()
        );

        for (std::size_t index = 0; index < resultLimit; ++index)
        {
            const DifferentialExpressionResult& result =
                classifiedExpressionResults.at(index);

            resultTable.rows.push_back({
                result.getGeneName(),
                QString::number(result.getControlMean(), 'g', 7).toStdString(),
                QString::number(result.getTreatmentMean(), 'g', 7).toStdString(),
                QString::number(result.getLog2FoldChange(), 'g', 7).toStdString(),
                QString::number(result.getPValue(), 'g', 7).toStdString(),
                QString::number(result.getAdjustedPValue(), 'g', 7).toStdString(),
                result.getRegulationName()
            });
        }

        report.tables.push_back(resultTable);

        if (!enrichmentResults.empty())
        {
            ReportTable pathwayTable;
            pathwayTable.title = "Functional Enrichment Results";
            pathwayTable.headers = {
                "Pathway", "Category", "Overlap", "Fold Enrichment",
                "Adjusted P-value", "Contributing Genes"
            };

            std::size_t pathwayLimit = std::min<std::size_t>(
                30,
                enrichmentResults.size()
            );

            for (std::size_t index = 0; index < pathwayLimit; ++index)
            {
                const EnrichmentResult& result = enrichmentResults.at(index);
                pathwayTable.rows.push_back({
                    result.getPathwayName(),
                    result.getCategory(),
                    std::to_string(result.getOverlapCount()) + " / "
                        + std::to_string(result.getPathwayGeneCount()),
                    QString::number(
                        result.getFoldEnrichment(), 'g', 6
                    ).toStdString(),
                    QString::number(
                        result.getAdjustedPValue(), 'g', 7
                    ).toStdString(),
                    result.getOverlappingGeneList()
                });
            }

            report.tables.push_back(pathwayTable);
        }

        volcanoPlotWidget->setResults(
            classifiedExpressionResults,
            adjustedPThresholdBox->value(),
            foldChangeThresholdBox->value()
        );

        QString volcanoPath = reportDirectory.filePath("volcano_plot.png");

        if (saveReportImage(volcanoPlotWidget, volcanoPath))
        {
            report.images.push_back({
                "Volcano Plot", "volcano_plot.png",
                "Genes are positioned by log2 fold change and statistical "
                "significance."
            });
        }

        const auto& heatmapGenes = filteredExpressionResults.empty()
            ? classifiedExpressionResults
            : filteredExpressionResults;

        expressionHeatmapWidget->setData(
            *expressionDataset,
            currentNormalizedExpressionValues,
            heatmapGenes,
            sampleGrouping,
            50
        );

        QString heatmapPath = reportDirectory.filePath(
            "expression_heatmap.png"
        );

        if (saveReportImage(expressionHeatmapWidget, heatmapPath, 1200, 760))
        {
            report.images.push_back({
                "Gene-Expression Heatmap", "expression_heatmap.png",
                "Row Z-scores show relative expression patterns across samples."
            });
        }

        PCAAnalyzer pcaAnalyzer;
        PCAResult pcaResult = pcaAnalyzer.analyze(
            *expressionDataset,
            currentNormalizedExpressionValues
        );
        pcaPlotWidget->setResult(pcaResult, sampleGrouping);

        QString pcaPath = reportDirectory.filePath("pca_sample_plot.png");

        if (saveReportImage(pcaPlotWidget, pcaPath))
        {
            report.images.push_back({
                "PCA Sample Plot", "pca_sample_plot.png",
                "Nearby samples have similar overall expression profiles."
            });
        }

        if (!enrichmentResults.empty())
        {
            enrichmentBarChart->setResults(enrichmentResults, 10);
            QString pathwayPath = reportDirectory.filePath(
                "pathway_enrichment_chart.png"
            );

            if (saveReportImage(enrichmentBarChart, pathwayPath))
            {
                report.images.push_back({
                    "Pathway Enrichment", "pathway_enrichment_chart.png",
                    "Bars show -log10 adjusted p-values for ranked pathways."
                });
            }
        }

        report.notes = {
            "Statistical association does not by itself demonstrate biological "
            "causation; important results should be validated experimentally.",
            "Welch's t-test assumes independent biological samples and should "
            "not be used as a replacement for count-specific RNA-seq models.",
            "The built-in BioFlow pathway collection is an educational curated "
            "dataset and is not the complete official GO, KEGG or Reactome database.",
            "Tables in this report are limited to the first 100 genes and 30 pathways."
        };

        QString reportPath = reportDirectory.filePath("index.html");
        HtmlReportGenerator generator;
        generator.generate(reportPath.toStdString(), report);

        QMessageBox::information(
            this,
            "Report Generated",
            "The complete expression-analysis report was generated.\n\n"
            "Report: " + reportPath
        );

        QDesktopServices::openUrl(QUrl::fromLocalFile(reportPath));
    }
    catch (const std::exception& error)
    {
        QMessageBox::critical(
            this,
            "Report Generation Error",
            QString::fromStdString(error.what())
        );
    }
}

void MainWindow::generatePhylogeneticHtmlReport()
{
    const DistanceMatrix* availableMatrix = nullptr;

    if (currentDistanceMatrix.size() > 0)
    {
        availableMatrix = &currentDistanceMatrix;
    }
    else if (treeDistanceMatrix.size() > 0)
    {
        availableMatrix = &treeDistanceMatrix;
    }

    if (loadedSequences.empty()
        || (availableMatrix == nullptr && currentTree.isEmpty()))
    {
        QMessageBox::warning(
            this,
            "No Phylogenetic Results",
            "Generate a distance matrix or phylogenetic tree before creating "
            "the report."
        );
        return;
    }

    QString parentPath = QFileDialog::getExistingDirectory(
        this,
        "Select Folder for Phylogenetic Report",
        QString(),
        QFileDialog::ShowDirsOnly
    );

    if (parentPath.isEmpty())
    {
        return;
    }

    try
    {
        QString timeStamp = QDateTime::currentDateTime().toString(
            "yyyyMMdd_HHmmss"
        );
        QString folderName = "BioFlow_Phylogenetic_Report_" + timeStamp;
        QDir parentDirectory(parentPath);

        if (!parentDirectory.mkpath(folderName))
        {
            throw std::runtime_error("Could not create the report folder.");
        }

        QDir reportDirectory(parentDirectory.filePath(folderName));
        AnalysisReport report;
        report.title = "BioFlow Studio Phylogenetic Analysis Report";
        report.subtitle = "Sequence quality, pairwise distances and UPGMA tree";
        report.generatedAt = QDateTime::currentDateTime()
            .toString("dddd, dd MMMM yyyy — hh:mm AP")
            .toStdString();
        report.overview =
            "This automatically generated report summarizes the imported "
            "biological sequences, quality assessment, pairwise distance "
            "calculation and phylogenetic reconstruction.";

        QString method = !currentTree.isEmpty()
            ? treeAlignmentMethodBox->currentText()
            : matrixAlignmentMethodBox->currentText();

        report.metadata = {
            {"Input Files", std::to_string(selectedFastaFiles.size())},
            {"Sequences", std::to_string(loadedSequences.size())},
            {"Distance Method", method.toStdString()},
            {"Tree Method", currentTree.isEmpty()
                ? "Not generated" : "UPGMA"},
            {"Distance Matrix", availableMatrix == nullptr
                ? "Not generated" : "Available"}
        };

        std::vector<std::string> sourceFiles;

        for (const QString& filePath : selectedFastaFiles)
        {
            sourceFiles.push_back(filePath.toStdString());
        }

        SequenceQualityAnalyzer qualityAnalyzer(
            loadedSequences,
            sourceFiles
        );
        QualityReport qualityReport = qualityAnalyzer.analyze();
        ReportTable qualityTable;
        qualityTable.title = "FASTA Data-Quality Assessment";
        qualityTable.headers = {
            "Check", "Status", "Observation", "Recommendation"
        };

        for (const QualityCheck& check : qualityReport.getChecks())
        {
            qualityTable.rows.push_back({
                check.name,
                QualityReport::statusName(check.status),
                check.observation,
                check.recommendation
            });
        }

        report.tables.push_back(qualityTable);

        ReportTable sequenceTable;
        sequenceTable.title = "Imported Sequences";
        sequenceTable.headers = {"Identifier", "Type", "Length"};

        for (const auto& sequence : loadedSequences)
        {
            sequenceTable.rows.push_back({
                sequence->getIdentifier(),
                sequence->getTypeName(),
                std::to_string(sequence->getLength())
            });
        }

        report.tables.push_back(sequenceTable);

        if (availableMatrix != nullptr)
        {
            ReportTable matrixTable;
            matrixTable.title = "Pairwise Distance Matrix";
            matrixTable.headers.push_back("Sequence");

            for (const std::string& label : availableMatrix->getLabels())
            {
                matrixTable.headers.push_back(label);
            }

            for (std::size_t row = 0; row < availableMatrix->size(); ++row)
            {
                std::vector<std::string> values;
                values.push_back(availableMatrix->getLabels().at(row));

                for (std::size_t column = 0;
                     column < availableMatrix->size();
                     ++column)
                {
                    values.push_back(
                        QString::number(
                            availableMatrix->getDistance(row, column),
                            'f', 4
                        ).toStdString()
                    );
                }

                matrixTable.rows.push_back(values);
            }

            report.tables.push_back(matrixTable);
        }

        if (!currentTree.isEmpty())
        {
            ReportTable newickTable;
            newickTable.title = "Newick Tree Representation";
            newickTable.headers = {"Newick"};
            newickTable.rows = {{currentTree.toNewick()}};
            report.tables.push_back(newickTable);

            QString treePath = reportDirectory.filePath(
                "phylogenetic_tree.png"
            );

            if (saveReportImage(treeGraphic, treePath, 1200, 760))
            {
                report.images.push_back({
                    "UPGMA Phylogenetic Tree", "phylogenetic_tree.png",
                    "Branching summarizes similarity derived from the selected "
                    "pairwise distance strategy."
                });
            }
        }

        report.notes = {
            "UPGMA assumes an approximately constant evolutionary rate among "
            "lineages; this assumption may not hold for every dataset.",
            "Tree topology and branch lengths depend on sequence quality, "
            "alignment strategy and distance definition.",
            "This educational analysis should be confirmed with established "
            "phylogenetic software before research publication."
        };

        QString reportPath = reportDirectory.filePath("index.html");
        HtmlReportGenerator generator;
        generator.generate(reportPath.toStdString(), report);

        QMessageBox::information(
            this,
            "Report Generated",
            "The phylogenetic analysis report was generated.\n\n"
            "Report: " + reportPath
        );

        QDesktopServices::openUrl(QUrl::fromLocalFile(reportPath));
    }
    catch (const std::exception& error)
    {
        QMessageBox::critical(
            this,
            "Report Generation Error",
            QString::fromStdString(error.what())
        );
    }
}

void MainWindow::exportWidgetImage(
    QWidget* widget,
    const QString& suggestedFileName,
    const QString& dialogTitle
)
{
    if (widget == nullptr)
    {
        return;
    }

    QString filePath = QFileDialog::getSaveFileName(
        this,
        dialogTitle,
        suggestedFileName,
        "PNG Images (*.png)"
    );

    if (filePath.isEmpty())
    {
        return;
    }

    if (!filePath.endsWith(".png", Qt::CaseInsensitive))
    {
        filePath += ".png";
    }

    QPixmap image = widget->grab();

    if (image.isNull() || !image.save(filePath, "PNG"))
    {
        QMessageBox::critical(
            this,
            "Image Export Error",
            "The visualization could not be saved as a PNG image."
        );
        return;
    }

    QMessageBox::information(
        this,
        "Image Exported",
        "Visualization saved successfully:\n" + filePath
    );
}
