#include "MainWindow.h"
#include <QFileDialog>
#include <QFileInfo>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>
#include <QFileInfo>
#include <QListWidget>
#include <QMessageBox>

#include <exception>
#include <utility>

#include "phylogenetics/FastaParser.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      currentProject(
          "BioFlow Demonstration",
          "A workflow platform for biological data analysis",
          WorkspaceType::NotSelected
      ),
      pages(new QStackedWidget(this)),
      statusLabel(nullptr)
{
    setWindowTitle("BioFlow Studio");
    resize(1000, 650);
    setMinimumSize(800, 550);

    pages->addWidget(createDashboardPage());
    pages->addWidget(createPhylogeneticPage());
    pages->addWidget(createGeneExpressionPage());

    setCentralWidget(pages);

    setStyleSheet(
        "QWidget {"
        "    background-color: #F4F7FA;"
        "    font-family: Arial;"
        "    font-size: 16px;"
        "    color: #263645;"
        "}"
        "QLabel {"
        "    background-color: transparent;"
        "    color: #263645;"
        "}"
        "#titleLabel {"
        "    color: #163A5F;"
        "    font-size: 38px;"
        "    font-weight: bold;"
        "}"
        "#pageTitle {"
        "    color: #163A5F;"
        "    font-size: 30px;"
        "    font-weight: bold;"
        "}"
        "#subtitleLabel {"
        "    color: #40566B;"
        "    font-size: 20px;"
        "}"
        "#descriptionLabel {"
        "    color: #263645;"
        "    font-size: 16px;"
        "}"
        "QPushButton {"
        "    background-color: #163A5F;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 8px;"
        "    font-size: 17px;"
        "    font-weight: bold;"
        "    padding: 14px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #2A6797;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #0F2B47;"
        "}"
        "#backButton {"
        "    background-color: #5D6D7E;"
        "}"
        "#statusLabel {"
        "    color: #2D6A4F;"
        "    font-size: 16px;"
        "    font-weight: bold;"
        "}"
                "QListWidget {"
        "    background-color: white;"
        "    border: 1px solid #B8C4CE;"
        "    border-radius: 6px;"
        "    padding: 8px;"
        "}"
        "#fileLabel {"
        "    background-color: white;"
        "    border: 1px solid #B8C4CE;"
        "    border-radius: 6px;"
        "    padding: 20px;"
        "    color: #263645;"
        "}"
    );
}

QWidget* MainWindow::createDashboardPage()
{
    QWidget* page = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(page);

    layout->setContentsMargins(80, 55, 80, 55);
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
        "Choose one of the two bioinformatics workspaces to begin an analysis."
    );
    description->setObjectName("descriptionLabel");
    description->setAlignment(Qt::AlignCenter);
    description->setWordWrap(true);

    QPushButton* phylogeneticButton =
        new QPushButton("Phylogenetic Analysis");

    QPushButton* expressionButton =
        new QPushButton("Gene Expression Analysis");

    phylogeneticButton->setMinimumHeight(65);
    expressionButton->setMinimumHeight(65);

    statusLabel = new QLabel("No workspace selected");
    statusLabel->setObjectName("statusLabel");
    statusLabel->setAlignment(Qt::AlignCenter);

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

    layout->addStretch();
    layout->addWidget(title);
    layout->addWidget(subtitle);
    layout->addWidget(description);
    layout->addSpacing(20);
    layout->addWidget(phylogeneticButton);
    layout->addWidget(expressionButton);
    layout->addSpacing(10);
    layout->addWidget(statusLabel);
    layout->addStretch();

    return page;
}

QWidget* MainWindow::createPhylogeneticPage()
{
    QWidget* page = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(page);

    layout->setContentsMargins(70, 45, 70, 45);
    layout->setSpacing(18);

    QLabel* title = new QLabel("Phylogenetic Analysis");
    title->setObjectName("pageTitle");
    title->setAlignment(Qt::AlignCenter);

    QLabel* description = new QLabel(
        "Import multiple FASTA files containing DNA or protein sequences. "
        "The files will later pass through validation, alignment, distance "
        "calculation and phylogenetic tree construction."
    );
    description->setObjectName("descriptionLabel");
    description->setAlignment(Qt::AlignCenter);
    description->setWordWrap(true);

    QPushButton* importButton =
        new QPushButton("Select FASTA Files");

    phylogeneticFileList = new QListWidget;
    phylogeneticFileList->setMinimumHeight(180);
    phylogeneticFileList->addItem("No FASTA files selected");

    QPushButton* backButton =
        new QPushButton("Back to Dashboard");

    backButton->setObjectName("backButton");

    connect(
        importButton,
        &QPushButton::clicked,
        this,
        &MainWindow::importFastaFiles
    );

    connect(
        backButton,
        &QPushButton::clicked,
        this,
        &MainWindow::returnToDashboard
    );

    layout->addWidget(title);
    layout->addWidget(description);
    layout->addSpacing(10);
    layout->addWidget(importButton);
    layout->addWidget(phylogeneticFileList);
    layout->addStretch();
    layout->addWidget(backButton);

    return page;
}

QWidget* MainWindow::createGeneExpressionPage()
{
    QWidget* page = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(page);

    layout->setContentsMargins(70, 45, 70, 45);
    layout->setSpacing(18);

    QLabel* title = new QLabel("Gene Expression Analysis");
    title->setObjectName("pageTitle");
    title->setAlignment(Qt::AlignCenter);

    QLabel* description = new QLabel(
        "Import a CSV or TSV gene-expression matrix. The dataset will later "
        "pass through quality control, normalization, PCA, differential "
        "expression analysis and candidate biomarker ranking."
    );
    description->setObjectName("descriptionLabel");
    description->setAlignment(Qt::AlignCenter);
    description->setWordWrap(true);

    QPushButton* importButton =
        new QPushButton("Select Expression File");

    expressionFileLabel =
        new QLabel("No expression file selected");

    expressionFileLabel->setObjectName("fileLabel");
    expressionFileLabel->setAlignment(Qt::AlignCenter);
    expressionFileLabel->setWordWrap(true);
    expressionFileLabel->setMinimumHeight(100);

    QPushButton* backButton =
        new QPushButton("Back to Dashboard");

    backButton->setObjectName("backButton");

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

    layout->addWidget(title);
    layout->addWidget(description);
    layout->addSpacing(20);
    layout->addWidget(importButton);
    layout->addWidget(expressionFileLabel);
    layout->addStretch();
    layout->addWidget(backButton);

    return page;
}
void MainWindow::importFastaFiles()
{
    QStringList filePaths = QFileDialog::getOpenFileNames(
        this,
        "Select FASTA Files",
        QString(),
        "FASTA Files (*.fasta *.fa *.fna *.faa);;All Files (*.*)"
    );

    if (filePaths.isEmpty())
    {
        return;
    }

    FastaParser parser;

    selectedFastaFiles = filePaths;
    loadedSequences.clear();
    phylogeneticFileList->clear();

    try
    {
        for (const QString& filePath : selectedFastaFiles)
        {
            QFileInfo fileInformation(filePath);

            phylogeneticFileList->addItem(
                "FILE: " + fileInformation.fileName()
            );

            auto parsedSequences =
                parser.parseFile(filePath.toStdString());

            for (auto& sequence : parsedSequences)
            {
                QString sequenceInformation =
                    QString("    %1 | %2 | Length: %3")
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
                "Successfully loaded %1 biological sequence(s)"
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

        QMessageBox::critical(
            this,
            "FASTA Import Error",
            QString::fromStdString(error.what())
        );

        phylogeneticFileList->clear();
        phylogeneticFileList->addItem(
            "FASTA import failed. Please check the selected file."
        );
    }
}

void MainWindow::importExpressionFile()
{
    QString filePath = QFileDialog::getOpenFileName(
        this,
        "Select Gene Expression File",
        QString(),
        "Expression Files (*.csv *.tsv);;All Files (*.*)"
    );

    if (filePath.isEmpty())
    {
        return;
    }

    selectedExpressionFile = filePath;

    QFileInfo fileInformation(filePath);

    expressionFileLabel->setText(
        "Selected file: " + fileInformation.fileName()
    );

    expressionFileLabel->setToolTip(filePath);

    statusLabel->setText("Expression file selected");
}

void MainWindow::selectPhylogeneticWorkspace()
{
    currentProject.setWorkspaceType(
        WorkspaceType::PhylogeneticAnalysis
    );

    statusLabel->setText(
        "Selected workspace: " +
        QString::fromStdString(currentProject.getWorkspaceName())
    );

    pages->setCurrentIndex(1);
}

void MainWindow::selectGeneExpressionWorkspace()
{
    currentProject.setWorkspaceType(
        WorkspaceType::GeneExpressionAnalysis
    );

    statusLabel->setText(
        "Selected workspace: " +
        QString::fromStdString(currentProject.getWorkspaceName())
    );

    pages->setCurrentIndex(2);
}

void MainWindow::returnToDashboard()
{
    pages->setCurrentIndex(0);
}