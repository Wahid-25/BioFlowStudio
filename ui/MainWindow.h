#pragma once
#include <memory>
#include <vector>
#include "phylogenetics/Sequence.h"
#include <QMainWindow>
#include <QString>
#include <QStringList>
#include "phylogenetics/UPGMATree.h"
#include "phylogenetics/DistanceMatrix.h"

#include "core/Project.h"
class QComboBox;
class QPlainTextEdit;
class QTabWidget;
class QTableWidget;
class QLabel;
class QListWidget;
class QStackedWidget;
class QWidget;
class PhylogeneticTreeWidget;

class MainWindow : public QMainWindow
{
public:
    explicit MainWindow(QWidget* parent = nullptr);
    

private:
    Project currentProject;

    QStackedWidget* pages;
    QLabel* statusLabel;
    QTabWidget* analysisTabs = nullptr;
    QPlainTextEdit* treeOutput = nullptr;
    PhylogeneticTreeWidget* treeGraphic = nullptr;
    QLabel* treeStatusLabel = nullptr;

    UPGMATree currentTree; 
    QListWidget* phylogeneticFileList = nullptr;
    QLabel* expressionFileLabel = nullptr;

    QStringList selectedFastaFiles;
    QString selectedExpressionFile;
    std::vector<std::unique_ptr<Sequence>> loadedSequences;

    QWidget* createDashboardPage();
    QWidget* createPhylogeneticPage();
    QWidget* createGeneExpressionPage();
    QComboBox* alignmentMethodBox = nullptr;
    QTableWidget* distanceMatrixTable = nullptr;
    QLabel* matrixStatusLabel = nullptr;

    DistanceMatrix currentDistanceMatrix;
    void generateDistanceMatrix();
    void selectPhylogeneticWorkspace();
    void selectGeneExpressionWorkspace();
    void returnToDashboard();
    void generatePhylogeneticTree();
    void importFastaFiles();
    void importExpressionFile();
};