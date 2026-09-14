#include <QApplication>
#include <QLabel>
#include <QMainWindow>
#include <QObject>
#include <QPushButton>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>

#include "core/Project.h"

int main(int argc, char* argv[])
{
    QApplication application(argc, argv);

    Project currentProject(
        "BioFlow Demonstration",
        "A workflow platform for biological data analysis",
        WorkspaceType::NotSelected
    );

    QMainWindow window;
    window.setWindowTitle("BioFlow Studio");
    window.resize(1000, 650);

    QWidget* centralWidget = new QWidget;
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);

    mainLayout->setContentsMargins(80, 60, 80, 60);
    mainLayout->setSpacing(22);

    QLabel* titleLabel = new QLabel("BioFlow Studio");
    titleLabel->setObjectName("titleLabel");
    titleLabel->setAlignment(Qt::AlignCenter);

    QLabel* subtitleLabel = new QLabel(
        "Object-Oriented Bioinformatics Workflow Platform"
    );
    subtitleLabel->setObjectName("subtitleLabel");
    subtitleLabel->setAlignment(Qt::AlignCenter);

    QLabel* descriptionLabel = new QLabel(
        "Choose one of the two bioinformatics workspaces to begin an analysis."
    );
    descriptionLabel->setWordWrap(true);
    descriptionLabel->setAlignment(Qt::AlignCenter);

    QPushButton* phylogeneticButton =
        new QPushButton("Phylogenetic Analysis");

    QPushButton* expressionButton =
        new QPushButton("Gene Expression Analysis");

    phylogeneticButton->setMinimumHeight(65);
    expressionButton->setMinimumHeight(65);

    QLabel* statusLabel = new QLabel("No workspace selected");
    statusLabel->setObjectName("statusLabel");
    statusLabel->setAlignment(Qt::AlignCenter);

    mainLayout->addStretch();
    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(subtitleLabel);
    mainLayout->addWidget(descriptionLabel);
    mainLayout->addSpacing(20);
    mainLayout->addWidget(phylogeneticButton);
    mainLayout->addWidget(expressionButton);
    mainLayout->addSpacing(15);
    mainLayout->addWidget(statusLabel);
    mainLayout->addStretch();

    QObject::connect(
        phylogeneticButton,
        &QPushButton::clicked,
        [&currentProject, statusLabel]()
        {
            currentProject.setWorkspaceType(
                WorkspaceType::PhylogeneticAnalysis
            );

            statusLabel->setText(
                "Selected workspace: " +
                QString::fromStdString(currentProject.getWorkspaceName())
            );
        }
    );

    QObject::connect(
        expressionButton,
        &QPushButton::clicked,
        [&currentProject, statusLabel]()
        {
            currentProject.setWorkspaceType(
                WorkspaceType::GeneExpressionAnalysis
            );

            statusLabel->setText(
                "Selected workspace: " +
                QString::fromStdString(currentProject.getWorkspaceName())
            );
        }
    );

    centralWidget->setStyleSheet(
        "QWidget {"
        "    background-color: #F4F7FA;"
        "    font-family: Arial;"
        "    font-size: 16px;"
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
        "QPushButton {"
        "    background-color: #163A5F;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 8px;"
        "    font-size: 18px;"
        "    font-weight: bold;"
        "    padding: 14px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #245C8A;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #0F2B47;"
        "}"
        "#statusLabel {"
        "    color: #2D6A4F;"
        "    font-size: 16px;"
        "    font-weight: bold;"
        "}"
    );

    window.setCentralWidget(centralWidget);
    window.show();

    return application.exec();
}