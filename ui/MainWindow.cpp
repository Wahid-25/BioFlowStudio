#include "MainWindow.h"

#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>

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

    layout->setContentsMargins(80, 60, 80, 60);
    layout->setSpacing(20);

    QLabel* title = new QLabel("Phylogenetic Analysis");
    title->setObjectName("pageTitle");
    title->setAlignment(Qt::AlignCenter);

    QLabel* description = new QLabel(
        "This workspace will contain FASTA import, sequence validation, "
        "pairwise alignment, distance matrices, heatmaps and phylogenetic trees."
    );
    description->setObjectName("descriptionLabel");
    description->setAlignment(Qt::AlignCenter);
    description->setWordWrap(true);

    QPushButton* backButton = new QPushButton("Back to Dashboard");
    backButton->setObjectName("backButton");

    connect(
        backButton,
        &QPushButton::clicked,
        this,
        &MainWindow::returnToDashboard
    );

    layout->addStretch();
    layout->addWidget(title);
    layout->addWidget(description);
    layout->addSpacing(30);
    layout->addWidget(backButton);
    layout->addStretch();

    return page;
}

QWidget* MainWindow::createGeneExpressionPage()
{
    QWidget* page = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(page);

    layout->setContentsMargins(80, 60, 80, 60);
    layout->setSpacing(20);

    QLabel* title = new QLabel("Gene Expression Analysis");
    title->setObjectName("pageTitle");
    title->setAlignment(Qt::AlignCenter);

    QLabel* description = new QLabel(
        "This workspace will contain expression data import, normalization, "
        "correlation, PCA, differential expression, volcano plots, heatmaps "
        "and candidate biomarker ranking."
    );
    description->setObjectName("descriptionLabel");
    description->setAlignment(Qt::AlignCenter);
    description->setWordWrap(true);

    QPushButton* backButton = new QPushButton("Back to Dashboard");
    backButton->setObjectName("backButton");

    connect(
        backButton,
        &QPushButton::clicked,
        this,
        &MainWindow::returnToDashboard
    );

    layout->addStretch();
    layout->addWidget(title);
    layout->addWidget(description);
    layout->addSpacing(30);
    layout->addWidget(backButton);
    layout->addStretch();

    return page;
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