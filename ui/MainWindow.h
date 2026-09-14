#pragma once

#include <QMainWindow>
#include <QString>
#include <QStringList>

#include "core/Project.h"

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

    QWidget* createDashboardPage();
    QWidget* createPhylogeneticPage();
    QWidget* createGeneExpressionPage();

    void selectPhylogeneticWorkspace();
    void selectGeneExpressionWorkspace();
    void returnToDashboard();

    void importFastaFiles();
    void importExpressionFile();
};