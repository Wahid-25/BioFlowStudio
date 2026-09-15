#pragma once

#include <QWidget>

#include "phylogenetics/UPGMATree.h"

class QPaintEvent;

class PhylogeneticTreeWidget : public QWidget
{
private:
    const UPGMATree* tree = nullptr;

protected:
    void paintEvent(QPaintEvent* event) override;

public:
    explicit PhylogeneticTreeWidget(
        QWidget* parent = nullptr
    );

    void setTree(const UPGMATree* newTree);
    void clearTree();
};