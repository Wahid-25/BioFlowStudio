#include "MainWindow.h"

#include <QAbstractItemView>
#include <QComboBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QHeaderView>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QStackedWidget>
#include <QStringList>
#include <QTabWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <QWidget>

#include <exception>
#include <memory>
#include <utility>

#include "phylogenetics/FastaParser.h"
#include "phylogenetics/PairwiseAligner.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      currentProject(
          "BioFlow Demonstration",
          "A workflow platform for biological data analysis",
          WorkspaceType::NotSelected
      ),
      pages(new QStackedWidget),
      statusLabel(nullptr)
{
    setWindowTitle("BioFlow Studio");
    resize(1000, 680);

    pages->addWidget(createDashboardPage());
    pages->addWidget(createPhylogeneticPage());
    pages->addWidget(createGeneExpressionPage());

    pages->setCurrentIndex(0);
    setCentralWidget(pages);

    setStyleSheet(
        "QWidget {"
        "    background-color: #F4F7FA;"
        "    font-family: Arial;"
        "    font-size: 15px;"
        "}"

        "#titleLabel {"
        "    color: #163A5F;"
        "    font-size: 38px;"
        "    font-weight: bold;"
        "}"

        "#subtitleLabel {"
        "    color: #40566B;"
        "    font-size: 20px;"
        "}"

        "#descriptionLabel {"
        "    color: #40566B;"
        "    font-size: 16px;"
        "}"

        "QPushButton {"
        "    background-color: #163A5F;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 8px;"
        "    font-size: 17px;"
        "    font-weight: bold;"
        "    padding: 10px;"
        "}"

        "QPushButton:hover {"
        "    background-color: #245C8A;"
        "}"

        "QPushButton:pressed {"
        "    background-color: #0F2B47;"
        "}"

        "QListWidget {"
        "    background-color: white;"
        "    color: #203040;"
        "    border: 1px solid #D7E0E8;"
        "    border-radius: 5px;"
        "    padding: 8px;"
        "}"

        "QComboBox {"
        "    background-color: white;"
        "    color: #203040;"
        "    border: 1px solid #BCC9D4;"
        "    border-radius: 5px;"
        "    padding: 7px;"
        "}"

        "QTableWidget {"
        "    background-color: white;"
        "    alternate-background-color: #EDF3F8;"
        "    color: #203040;"
        "    border: 1px solid #D7E0E8;"
        "    gridline-color: #CBD5DF;"
        "}"

        "QHeaderView::section {"
        "    background-color: #E3EBF2;"
        "    color: #163A5F;"
        "    font-weight: bold;"
        "    padding: 6px;"
        "    border: 1px solid #CBD5DF;"
        "}"

        "QTabWidget::pane {"
        "    border: 1px solid #BCC9D4;"
        "    background-color: white;"
        "}"

        "QTabBar::tab {"
        "    background-color: #DCE6EF;"
        "    color: #163A5F;"
        "    padding: 9px 18px;"
        "    font-weight: bold;"
        "}"

        "QTabBar::tab:selected {"
        "    background-color: #163A5F;"
        "    color: white;"
        "}"

        "QPlainTextEdit {"
        "    background-color: white;"
        "    color: #203040;"
        "    border: 1px solid #D7E0E8;"
        "    font-family: Consolas;"
        "    font-size: 14px;"
        "}"

        "#statusLabel {"
        "    color: #2D6A4F;"
        "    font-size: 16px;"
        "    font-weight: bold;"
        "}"
    );
}

QWidget* MainWindow::createDashboardPage()
{
    QWidget* page = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(page);

    layout->setContentsMargins(80, 55, 80, 55);
    layout->setSpacing(20);

    QLabel* titleLabel =
        new QLabel("BioFlow Studio");

    titleLabel->setObjectName("titleLabel");
    titleLabel->setAlignment(Qt::AlignCenter);

    QLabel* subtitleLabel = new QLabel(
        "Object-Oriented Bioinformatics Workflow Platform"
    );

    subtitleLabel->setObjectName("subtitleLabel");
    subtitleLabel->setAlignment(Qt::AlignCenter);

    QLabel* descriptionLabel = new QLabel(
        "Choose one of the two bioinformatics "
        "workspaces to begin an analysis."
    );

    descriptionLabel->setObjectName("descriptionLabel");
    descriptionLabel->setAlignment(Qt::AlignCenter);
    descriptionLabel->setWordWrap(true);

    QPushButton* phylogeneticButton =
        new QPushButton("Phylogenetic Analysis");

    QPushButton* expressionButton =
        new QPushButton("Gene Expression Analysis");

    phylogeneticButton->setMinimumHeight(60);
    expressionButton->setMinimumHeight(60);

    statusLabel = new QLabel("No workspace selected");
    statusLabel->setObjectName("statusLabel");
    statusLabel->setAlignment(Qt::AlignCenter);

    layout->addStretch();
    layout->addWidget(titleLabel);
    layout->addWidget(subtitleLabel);
    layout->addWidget(descriptionLabel);
    layout->addSpacing(18);
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

QWidget* MainWindow::createPhylogeneticPage()
{
    QWidget* page = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(page);

    layout->setContentsMargins(60, 18, 60, 18);
    layout->setSpacing(7);

    QLabel* title =
        new QLabel("Phylogenetic Analysis");

    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet(
        "font-size: 30px;"
        "font-weight: bold;"
        "color: #163A5F;"
    );

    QLabel* description = new QLabel(
        "Import biological sequences, select a pairwise "
        "distance algorithm, and generate a phylogenetic tree."
    );

    description->setAlignment(Qt::AlignCenter);
    description->setWordWrap(true);
    description->setStyleSheet("color: #40566B;");

    QPushButton* importButton =
        new QPushButton("Select FASTA Files");

    importButton->setMinimumHeight(42);

    phylogeneticFileList = new QListWidget;
    phylogeneticFileList->setMinimumHeight(80);
    phylogeneticFileList->setMaximumHeight(110);

    QLabel* methodLabel =
        new QLabel("Pairwise distance method:");

    methodLabel->setStyleSheet(
        "font-weight: bold;"
        "color: #163A5F;"
    );

    alignmentMethodBox = new QComboBox;

    alignmentMethodBox->addItem(
        "Needleman-Wunsch Global Distance"
    );

    alignmentMethodBox->addItem(
        "Hamming Distance"
    );

    QPushButton* generateButton =
        new QPushButton("Generate Distance Matrix");

    generateButton->setMinimumHeight(42);

    matrixStatusLabel =
        new QLabel("No distance matrix generated");

    matrixStatusLabel->setAlignment(Qt::AlignCenter);
    matrixStatusLabel->setStyleSheet(
        "font-weight: bold;"
        "color: #40566B;"
    );

    distanceMatrixTable = new QTableWidget;
    distanceMatrixTable->setMinimumHeight(125);
    distanceMatrixTable->setEditTriggers(
        QAbstractItemView::NoEditTriggers
    );

    distanceMatrixTable->setAlternatingRowColors(true);

    analysisTabs = new QTabWidget;

    analysisTabs->addTab(
        distanceMatrixTable,
        "Distance Matrix"
    );

    QWidget* treeTab = new QWidget;
    QVBoxLayout* treeLayout =
        new QVBoxLayout(treeTab);

    QPushButton* generateTreeButton =
        new QPushButton("Generate UPGMA Tree");

    generateTreeButton->setMinimumHeight(42);

    treeStatusLabel =
        new QLabel("Generate a distance matrix first");

    treeStatusLabel->setAlignment(Qt::AlignCenter);
    treeStatusLabel->setStyleSheet(
        "font-weight: bold;"
        "color: #40566B;"
    );

    treeOutput = new QPlainTextEdit;
    treeOutput->setReadOnly(true);
    treeOutput->setPlaceholderText(
        "The generated phylogenetic tree "
        "will appear here in Newick format."
    );

    treeLayout->addWidget(generateTreeButton);
    treeLayout->addWidget(treeStatusLabel);
    treeLayout->addWidget(treeOutput);

    analysisTabs->addTab(
        treeTab,
        "Phylogenetic Tree"
    );

    QPushButton* backButton =
        new QPushButton("Back to Dashboard");

    backButton->setMinimumHeight(42);

    layout->addWidget(title);
    layout->addWidget(description);
    layout->addWidget(importButton);
    layout->addWidget(phylogeneticFileList);
    layout->addWidget(methodLabel);
    layout->addWidget(alignmentMethodBox);
    layout->addWidget(generateButton);
    layout->addWidget(matrixStatusLabel);
    layout->addWidget(analysisTabs);
    layout->addWidget(backButton);

    layout->setStretchFactor(
        analysisTabs,
        1
    );

    connect(
        importButton,
        &QPushButton::clicked,
        this,
        &MainWindow::importFastaFiles
    );

    connect(
        generateButton,
        &QPushButton::clicked,
        this,
        &MainWindow::generateDistanceMatrix
    );

    connect(
        generateTreeButton,
        &QPushButton::clicked,
        this,
        &MainWindow::generatePhylogeneticTree
    );

    connect(
        backButton,
        &QPushButton::clicked,
        this,
        &MainWindow::returnToDashboard
    );

    return page;
}

QWidget* MainWindow::createGeneExpressionPage()
{
    QWidget* page = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(page);

    layout->setContentsMargins(80, 50, 80, 50);
    layout->setSpacing(18);

    QLabel* title =
        new QLabel("Gene Expression Analysis");

    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet(
        "font-size: 30px;"
        "font-weight: bold;"
        "color: #163A5F;"
    );

    QLabel* description = new QLabel(
        "Import a CSV or TSV gene-expression dataset. "
        "This workspace will perform normalization, "
        "differential-expression analysis, PCA, heatmaps, "
        "and volcano plots."
    );

    description->setAlignment(Qt::AlignCenter);
    description->setWordWrap(true);
    description->setStyleSheet("color: #40566B;");

    QPushButton* importButton =
        new QPushButton("Select Expression File");

    importButton->setMinimumHeight(55);

    expressionFileLabel =
        new QLabel("No expression file selected");

    expressionFileLabel->setAlignment(Qt::AlignCenter);
    expressionFileLabel->setWordWrap(true);
    expressionFileLabel->setStyleSheet(
        "background-color: white;"
        "color: #40566B;"
        "border: 1px solid #D7E0E8;"
        "border-radius: 5px;"
        "padding: 15px;"
    );

    QPushButton* backButton =
        new QPushButton("Back to Dashboard");

    backButton->setMinimumHeight(48);

    layout->addStretch();
    layout->addWidget(title);
    layout->addWidget(description);
    layout->addWidget(importButton);
    layout->addWidget(expressionFileLabel);
    layout->addStretch();
    layout->addWidget(backButton);

    connect(
        importButton,
        &QPushButton::clicked,
        this,
        &MainWindow::importExpressionFile
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

    pages->setCurrentIndex(1);
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

    pages->setCurrentIndex(2);
}

void MainWindow::returnToDashboard()
{
    pages->setCurrentIndex(0);
}

void MainWindow::importFastaFiles()
{
    QStringList filePaths = QFileDialog::getOpenFileNames(
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
    currentTree.clear();

    phylogeneticFileList->clear();
    distanceMatrixTable->clear();
    distanceMatrixTable->setRowCount(0);
    distanceMatrixTable->setColumnCount(0);

    treeOutput->clear();

    matrixStatusLabel->setText(
        "No distance matrix generated"
    );

    treeStatusLabel->setText(
        "Generate a distance matrix first"
    );

    try
    {
        for (const QString& filePath :
             selectedFastaFiles)
        {
            QFileInfo fileInformation(filePath);

            phylogeneticFileList->addItem(
                "FILE: " + fileInformation.fileName()
            );

            auto parsedSequences =
                parser.parseFile(
                    filePath.toStdString()
                );

            for (auto& sequence : parsedSequences)
            {
                QString sequenceInformation =
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
                    sequenceInformation
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
            QString::fromStdString(error.what())
        );
    }
}

void MainWindow::generateDistanceMatrix()
{
    if (loadedSequences.size() < 2)
    {
        QMessageBox::warning(
            this,
            "Insufficient Sequences",
            "Please import at least two "
            "compatible sequences."
        );

        return;
    }

    std::unique_ptr<PairwiseAligner> aligner;

    if (alignmentMethodBox->currentText()
        == "Hamming Distance")
    {
        aligner =
            std::make_unique<HammingAligner>();
    }
    else
    {
        aligner =
            std::make_unique<
                NeedlemanWunschAligner
            >();
    }

    try
    {
        currentDistanceMatrix.calculate(
            loadedSequences,
            *aligner
        );

        currentTree.clear();
        treeOutput->clear();

        treeStatusLabel->setText(
            "Generate the UPGMA tree"
        );

        std::size_t matrixSize =
            currentDistanceMatrix.size();

        distanceMatrixTable->clear();

        distanceMatrixTable->setRowCount(
            static_cast<int>(matrixSize)
        );

        distanceMatrixTable->setColumnCount(
            static_cast<int>(matrixSize)
        );

        QStringList labels;

        for (const std::string& label :
             currentDistanceMatrix.getLabels())
        {
            labels.append(
                QString::fromStdString(label)
            );
        }

        distanceMatrixTable->setHorizontalHeaderLabels(
            labels
        );

        distanceMatrixTable->setVerticalHeaderLabels(
            labels
        );

        for (std::size_t row = 0;
             row < matrixSize;
             ++row)
        {
            for (std::size_t column = 0;
                 column < matrixSize;
                 ++column)
            {
                double distance =
                    currentDistanceMatrix.getDistance(
                        row,
                        column
                    );

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
                QHeaderView::Stretch
            );

        matrixStatusLabel->setText(
            QString("Matrix generated using %1")
                .arg(
                    alignmentMethodBox->currentText()
                )
        );

        matrixStatusLabel->setStyleSheet(
            "font-weight: bold;"
            "color: #2D6A4F;"
        );

        analysisTabs->setCurrentIndex(0);
    }
    catch (const std::exception& error)
    {
        currentDistanceMatrix.clear();

        distanceMatrixTable->clear();
        distanceMatrixTable->setRowCount(0);
        distanceMatrixTable->setColumnCount(0);

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
            QString::fromStdString(error.what())
        );
    }
}

void MainWindow::generatePhylogeneticTree()
{
    if (currentDistanceMatrix.size() < 2)
    {
        QMessageBox::warning(
            this,
            "Distance Matrix Required",
            "Generate a distance matrix before "
            "building the phylogenetic tree."
        );

        return;
    }

    try
    {
        currentTree.build(currentDistanceMatrix);

        QString newickTree =
            QString::fromStdString(
                currentTree.toNewick()
            );

        treeOutput->setPlainText(newickTree);

        treeStatusLabel->setText(
            "UPGMA tree generated successfully"
        );

        treeStatusLabel->setStyleSheet(
            "font-weight: bold;"
            "color: #2D6A4F;"
        );

        analysisTabs->setCurrentIndex(1);
    }
    catch (const std::exception& error)
    {
        currentTree.clear();
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
            QString::fromStdString(error.what())
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

    selectedExpressionFile = filePath;

    QFileInfo fileInformation(filePath);

    expressionFileLabel->setText(
        "Selected file: "
        + fileInformation.fileName()
    );

    expressionFileLabel->setToolTip(filePath);

    statusLabel->setText(
        "Expression file selected"
    );
}