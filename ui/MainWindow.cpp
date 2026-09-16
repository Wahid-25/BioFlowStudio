#include "MainWindow.h"

#include <QAbstractItemView>
#include <QColor>
#include <QComboBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QPixmap>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QStackedWidget>
#include <QStringList>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <QWidget>
#include <QtCharts/QChart>

#include <algorithm>
#include <exception>
#include <memory>
#include <stdexcept>
#include <utility>

#include "export/PhylogeneticExporter.h"
#include "gene_expression/NormalizationStrategy.h"
#include "gene_expression/PCAAnalyzer.h"
#include "gene_expression/ExpressionParser.h"
#include "gene_expression/WelchTTestAnalyzer.h"
#include "phylogenetics/FastaParser.h"
#include "phylogenetics/PairwiseAligner.h"
#include "visualization/PhylogeneticTreeWidget.h"
#include "visualization/ExpressionHeatmapWidget.h"
#include "visualization/PCAPlotWidget.h"
#include "visualization/VolcanoPlotWidget.h"

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

    pages->setCurrentIndex(DashboardPage);
    setCentralWidget(pages);

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

    phylogeneticButton->setMinimumHeight(62);
    expressionButton->setMinimumHeight(62);

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

    openMatrixButton->setMinimumHeight(58);
    openTreeButton->setMinimumHeight(58);

    openMatrixButton->setEnabled(false);
    openTreeButton->setEnabled(false);

    QHBoxLayout* analysisOptions =
        new QHBoxLayout;

    analysisOptions->setSpacing(12);
    analysisOptions->addWidget(openMatrixButton);
    analysisOptions->addWidget(openTreeButton);

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

    generateButton->setMinimumHeight(45);
    exportButton->setMinimumHeight(45);

    QHBoxLayout* actionLayout =
        new QHBoxLayout;

    actionLayout->setSpacing(10);
    actionLayout->addWidget(generateButton);
    actionLayout->addWidget(exportButton);

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

    generateButton->setMinimumHeight(45);
    exportButton->setMinimumHeight(45);

    QHBoxLayout* actionLayout =
        new QHBoxLayout;

    actionLayout->setSpacing(10);
    actionLayout->addWidget(generateButton);
    actionLayout->addWidget(exportButton);

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

    QPushButton* backButton =
        new QPushButton("Back to Dashboard");

    backButton->setMinimumHeight(45);

    layout->addWidget(title);
    layout->addWidget(description);
    layout->addWidget(importButton);
    layout->addWidget(expressionFileLabel);
    layout->addWidget(expressionSummaryLabel);
    layout->addWidget(expressionPreviewTable, 1);
    layout->addWidget(configureExpressionButton);
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

    layout->addWidget(title);
    layout->addWidget(description);
    layout->addWidget(expressionResultsSummaryLabel);
    layout->addWidget(expressionResultsTable, 1);
    layout->addLayout(visualizationButtonLayout);
    layout->addWidget(backButton);

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

    QPushButton* backButton = new QPushButton(
        "Back to Differential Expression Results"
    );
    backButton->setMinimumHeight(44);

    QHBoxLayout* buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(resetZoomButton);
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

    layout->addWidget(title);
    layout->addWidget(description);
    layout->addWidget(expressionHeatmapSummaryLabel);
    layout->addLayout(legendLayout);
    layout->addWidget(expressionHeatmapWidget, 1);
    layout->addWidget(backButton);

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

    QPushButton* backButton = new QPushButton(
        "Back to Differential Expression Results"
    );
    backButton->setMinimumHeight(44);

    QHBoxLayout* buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(resetZoomButton);
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
        backButton,
        &QPushButton::clicked,
        this,
        &MainWindow::returnToExpressionResults
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
    if (expressionResults.empty())
    {
        QMessageBox::warning(
            this,
            "No Analysis Results",
            "Run differential expression analysis before opening "
            "the volcano plot."
        );
        return;
    }

    volcanoPlotWidget->setResults(expressionResults);
    pages->setCurrentIndex(ExpressionVolcanoPage);
}

void MainWindow::openExpressionHeatmapPage()
{
    if (!expressionDataset
        || expressionResults.empty()
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
            expressionResults,
            sampleGrouping,
            maximumDisplayedGenes
        );

        std::size_t displayedGenes = std::min(
            maximumDisplayedGenes,
            expressionDataset->getGeneCount()
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

    ExpressionParser parser;

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
        currentNormalizedExpressionValues.clear();
        configureExpressionButton->setEnabled(true);

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
    }
    catch (const std::exception& error)
    {
        expressionDataset.reset();
        sampleGrouping.clear();
        expressionResults.clear();
        currentNormalizedExpressionValues.clear();
        configureExpressionButton->setEnabled(false);

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

        populateExpressionResultsTable();
        pages->setCurrentIndex(ExpressionResultsPage);
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

void MainWindow::populateExpressionResultsTable()
{
    expressionResultsTable->setSortingEnabled(false);
    expressionResultsTable->clearContents();
    expressionResultsTable->setRowCount(
        static_cast<int>(expressionResults.size())
    );

    std::size_t upregulatedCount = 0;
    std::size_t downregulatedCount = 0;

    for (std::size_t row = 0; row < expressionResults.size(); ++row)
    {
        const DifferentialExpressionResult& result =
            expressionResults.at(row);

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
            ++upregulatedCount;
        }
        else if (result.getRegulationStatus() == RegulationStatus::Downregulated)
        {
            regulationItem->setBackground(QColor("#FFD6D6"));
            regulationItem->setForeground(QColor("#9C1C1C"));
            ++downregulatedCount;
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
        .arg(static_cast<qulonglong>(expressionResults.size()))
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
