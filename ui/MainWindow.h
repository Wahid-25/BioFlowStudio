#pragma once

#include <QMainWindow>

#include "core/Project.h"

class QLabel;
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

    QWidget* createDashboardPage();
    QWidget* createPhylogeneticPage();
    QWidget* createGeneExpressionPage();

    void selectPhylogeneticWorkspace();
    void selectGeneExpressionWorkspace();
    void returnToDashboard();
};