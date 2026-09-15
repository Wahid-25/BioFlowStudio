#include "PhylogeneticExporter.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <stdexcept>

void PhylogeneticExporter::exportCSV(
    const DistanceMatrix& matrix,
    const std::string& filePath
)
{
    if (matrix.size() == 0)
    {
        throw std::runtime_error(
            "No distance matrix is available for export."
        );
    }

    std::ofstream output(filePath);

    if (!output.is_open())
    {
        throw std::runtime_error(
            "Could not create CSV file."
        );
    }

    const auto& labels = matrix.getLabels();
    const auto& values = matrix.getValues();

    output << "Sequence";

    for (const std::string& label : labels)
    {
        output << ",\"" << label << "\"";
    }

    output << '\n';
    output << std::fixed << std::setprecision(6);

    for (std::size_t row = 0;
         row < values.size();
         ++row)
    {
        output << "\"" << labels[row] << "\"";

        for (double distance : values[row])
        {
            output << "," << distance;
        }

        output << '\n';
    }
}

void PhylogeneticExporter::exportPhylip(
    const DistanceMatrix& matrix,
    const std::string& filePath
)
{
    if (matrix.size() == 0)
    {
        throw std::runtime_error(
            "No distance matrix is available for export."
        );
    }

    std::ofstream output(filePath);

    if (!output.is_open())
    {
        throw std::runtime_error(
            "Could not create PHYLIP file."
        );
    }

    const auto& labels = matrix.getLabels();
    const auto& values = matrix.getValues();

    output << matrix.size() << '\n';
    output << std::fixed << std::setprecision(6);

    for (std::size_t row = 0;
         row < values.size();
         ++row)
    {
        std::string phylipLabel =
            createPhylipLabel(
                labels[row],
                row
            );

        output << std::left
               << std::setw(10)
               << phylipLabel
               << " ";

        output << std::right;

        for (double distance : values[row])
        {
            output << distance << " ";
        }

        output << '\n';
    }
}

void PhylogeneticExporter::exportNewick(
    const UPGMATree& tree,
    const std::string& filePath
)
{
    if (tree.isEmpty())
    {
        throw std::runtime_error(
            "No phylogenetic tree is available for export."
        );
    }

    std::ofstream output(filePath);

    if (!output.is_open())
    {
        throw std::runtime_error(
            "Could not create Newick file."
        );
    }

    output << tree.toNewick() << '\n';
}

std::string PhylogeneticExporter::createPhylipLabel(
    const std::string& label,
    std::size_t index
)
{
    std::string result = label;

    std::replace(
        result.begin(),
        result.end(),
        ' ',
        '_'
    );

    if (result.empty())
    {
        result =
            "Sequence" + std::to_string(index + 1);
    }

    if (result.length() > 10)
    {
        result = result.substr(0, 10);
    }

    return result;
}