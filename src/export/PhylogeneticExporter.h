#pragma once

#include <string>

#include "phylogenetics/DistanceMatrix.h"
#include "phylogenetics/UPGMATree.h"

class PhylogeneticExporter
{
public:
    static void exportCSV(
        const DistanceMatrix& matrix,
        const std::string& filePath
    );

    static void exportPhylip(
        const DistanceMatrix& matrix,
        const std::string& filePath
    );

    static void exportNewick(
        const UPGMATree& tree,
        const std::string& filePath
    );

private:
    static std::string createPhylipLabel(
        const std::string& label,
        std::size_t index
    );
};