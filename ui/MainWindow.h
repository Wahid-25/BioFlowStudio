#pragma once

#include <QMainWindow>
#include <QString>
#include <QStringList>

#include <memory>
#include <vector>

#include "core/Project.h"
#include "phylogenetics/DistanceMatrix.h"
#include "phylogenetics/Sequence.h"
#include "phylogenetics/UPGMATree.h"

class QLabel;
class QListWidget;
class QComboBox;
class QPlainTextEdit;
class QPushButton;
class QStackedWidget;
class QTableWidget;
class QWidget;

class PairwiseAligner;
class PhylogeneticTreeWidget;

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

    QLabel* expressionFileLabel = nullptr;
    QString selectedExpressionFile;

    QWidget* createDashboardPage();
    QWidget* createPhylogeneticSetupPage();
    QWidget* createDistanceMatrixPage();
    QWidget* createPhylogeneticTreePage();
    QWidget* createGeneExpressionPage();

    void selectPhylogeneticWorkspace();
    void selectGeneExpressionWorkspace();

    void openDistanceMatrixPage();
    void openPhylogeneticTreePage();

    void returnToDashboard();
    void returnToPhylogeneticSetup();

    void importFastaFiles();
    void importExpressionFile();

    void generateDistanceMatrix();
    void generatePhylogeneticTree();

    void exportMatrixResults();
    void exportTreeResults();

    void populateDistanceMatrixTable(
        const DistanceMatrix& matrix
    );

    std::unique_ptr<PairwiseAligner> createAligner(
        const QString& methodName
    ) const;
};