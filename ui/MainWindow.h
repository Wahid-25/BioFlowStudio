#pragma once
#include <memory>
#include <vector>
#include "phylogenetics/Sequence.h"
#include <QMainWindow>
#include <QString>
#include <QStringList>
#include "phylogenetics/DistanceMatrix.h"

#include "core/Project.h"
class QComboBox;
class QTableWidget;
class QLabel;
class QListWidget;
class QStackedWidget;
class QWidget;

class MainWindow : public QMainWindow
{
public:
    explicit MainWindow(QWidget* parent = nullptr);

private:
    Project currentProject;

    QStackedWidget* pages;
    QLabel* statusLabel;

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

    void importFastaFiles();
    void importExpressionFile();
};