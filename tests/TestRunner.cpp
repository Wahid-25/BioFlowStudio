#include <QCoreApplication>
#include <QTemporaryDir>

#include <cmath>
#include <exception>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "gene_expression/ExpressionDataset.h"
#include "gene_expression/ExpressionParser.h"
#include "gene_expression/NormalizationStrategy.h"
#include "gene_expression/SampleGrouping.h"
#include "gene_expression/WelchTTestAnalyzer.h"
#include "phylogenetics/DistanceMatrix.h"
#include "phylogenetics/FastaParser.h"
#include "phylogenetics/PairwiseAligner.h"
#include "session/ProjectSerializer.h"
#include "session/ProjectSession.h"

namespace
{
void require(bool condition, const std::string& message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

bool nearlyEqual(double first, double second, double tolerance = 1e-8)
{
    return std::abs(first - second) <= tolerance;
}

void writeTextFile(const QString& path, const std::string& contents)
{
    std::ofstream file(path.toStdString());

    if (!file)
    {
        throw std::runtime_error("Could not create temporary test data.");
    }

    file << contents;
}

void testFastaParserAndDistanceMatrix()
{
    QTemporaryDir directory;
    require(directory.isValid(), "Temporary directory was not created.");

    QString fastaPath = directory.filePath("distance_test.fasta");
    writeTextFile(
        fastaPath,
        ">Sequence_A\nAAAA\n"
        ">Sequence_B\nAAAT\n"
        ">Sequence_C\nAATT\n"
    );

    FastaParser parser;
    auto sequences = parser.parseFile(fastaPath.toStdString());

    require(sequences.size() == 3, "FASTA parser did not load three records.");
    require(sequences.at(0)->getIdentifier() == "Sequence_A",
            "The first FASTA identifier is incorrect.");
    require(sequences.at(0)->getLength() == 4,
            "The first FASTA sequence length is incorrect.");

    HammingAligner hamming;
    DistanceMatrix hammingMatrix;
    hammingMatrix.calculate(sequences, hamming);

    require(hammingMatrix.size() == 3,
            "Hamming distance matrix has the wrong size.");
    require(nearlyEqual(hammingMatrix.getDistance(0, 0), 0.0),
            "Distance-matrix diagonal must be zero.");
    require(nearlyEqual(
                hammingMatrix.getDistance(0, 1),
                hammingMatrix.getDistance(1, 0)
            ),
            "Distance matrix must be symmetric.");
    require(hammingMatrix.getDistance(0, 1) > 0.0,
            "Different sequences must have a positive Hamming distance.");
    require(hammingMatrix.getDistance(0, 2)
                >= hammingMatrix.getDistance(0, 1),
            "Two substitutions should not be closer than one substitution.");

    NeedlemanWunschAligner needlemanWunsch;
    DistanceMatrix globalMatrix;
    globalMatrix.calculate(sequences, needlemanWunsch);

    require(globalMatrix.size() == 3,
            "Needleman-Wunsch distance matrix has the wrong size.");
    require(nearlyEqual(globalMatrix.getDistance(2, 2), 0.0),
            "Global-alignment self-distance must be zero.");
}

void testExpressionPipeline()
{
    QTemporaryDir directory;
    require(directory.isValid(), "Temporary directory was not created.");

    QString csvPath = directory.filePath("expression_test.csv");
    writeTextFile(
        csvPath,
        "Gene,Control_1,Control_2,Treatment_1,Treatment_2\n"
        "GeneA,4,5,32,30\n"
        "GeneB,10,11,10,12\n"
        "GeneC,20,18,3,4\n"
    );

    ExpressionParser parser;
    ExpressionDataset dataset = parser.parseFile(csvPath.toStdString());

    require(dataset.getGeneCount() == 3,
            "Expression parser returned the wrong gene count.");
    require(dataset.getSampleCount() == 4,
            "Expression parser returned the wrong sample count.");
    require(nearlyEqual(dataset.getValue(0, 0), 4.0),
            "Expression value was parsed incorrectly.");

    RawNormalization rawNormalizer;
    auto rawValues = rawNormalizer.normalize(dataset);
    require(nearlyEqual(rawValues.at(0).at(0), 4.0),
            "Raw normalization changed an input value.");

    Log2Normalization logNormalizer;
    auto logValues = logNormalizer.normalize(dataset);
    require(nearlyEqual(logValues.at(0).at(0), std::log2(5.0)),
            "Log2 normalization does not match log2(x + 1).");

    ZScoreNormalization zScoreNormalizer;
    auto zScoreValues = zScoreNormalizer.normalize(dataset);
    require(zScoreValues.size() == dataset.getGeneCount(),
            "Z-score normalization returned the wrong number of genes.");

    SampleGrouping grouping;
    grouping.automaticallyAssign(dataset.getSampleNames());
    require(grouping.isValid(),
            "Control and treatment sample names were not grouped correctly.");
    require(grouping.getControlCount() == 2,
            "Control sample count is incorrect.");
    require(grouping.getTreatmentCount() == 2,
            "Treatment sample count is incorrect.");

    WelchTTestAnalyzer analyzer;
    auto results = analyzer.analyze(dataset, logValues, grouping);
    require(results.size() == dataset.getGeneCount(),
            "Differential-expression analysis returned the wrong result count.");

    for (const auto& result : results)
    {
        require(std::isfinite(result.getLog2FoldChange()),
                "A log2 fold change is not finite.");
        require(std::isfinite(result.getPValue()),
                "A p-value is not finite.");
        require(result.getPValue() >= 0.0 && result.getPValue() <= 1.0,
                "A p-value is outside the range zero to one.");
    }
}

void testProjectPersistence()
{
    QTemporaryDir directory;
    require(directory.isValid(), "Temporary directory was not created.");

    ProjectSession original;
    original.projectName = "Automated Test Project";
    original.savedAt = "2026-09-17T12:00:00";
    original.fastaFilePaths = {"alpha.fasta", "beta.fasta"};
    original.expressionFilePath = "expression.csv";
    original.matrixMethodIndex = 1;
    original.treeMethodIndex = 0;
    original.normalizationMethodIndex = 2;
    original.adjustedPValueThreshold = 0.01;
    original.foldChangeThreshold = 1.5;
    original.enrichmentPValueThreshold = 0.05;
    original.workflowTemplateIndex = 1;
    original.sampleGroups = {1, 1, 2, 2};
    original.addHistory(
        "Testing",
        "Round-trip Persistence",
        "Completed",
        "Automated serializer test"
    );

    QString projectPath = directory.filePath("test.bioflow");
    ProjectSerializer::save(projectPath, original);
    ProjectSession restored = ProjectSerializer::load(projectPath);

    require(restored.projectName == original.projectName,
            "Project name was not restored.");
    require(restored.fastaFilePaths == original.fastaFilePaths,
            "FASTA file references were not restored.");
    require(restored.expressionFilePath == original.expressionFilePath,
            "Expression file reference was not restored.");
    require(restored.normalizationMethodIndex
                == original.normalizationMethodIndex,
            "Normalization setting was not restored.");
    require(nearlyEqual(
                restored.adjustedPValueThreshold,
                original.adjustedPValueThreshold
            ),
            "Adjusted p-value threshold was not restored.");
    require(restored.sampleGroups == original.sampleGroups,
            "Sample assignments were not restored.");
    require(restored.history.size() == 1,
            "Project analysis history was not restored.");
}
}

int main(int argc, char* argv[])
{
    QCoreApplication application(argc, argv);

    struct TestCase
    {
        std::string name;
        std::function<void()> function;
    };

    const std::vector<TestCase> tests = {
        {"FASTA parsing and distance matrices", testFastaParserAndDistanceMatrix},
        {"Gene-expression pipeline", testExpressionPipeline},
        {"Project save/load round trip", testProjectPersistence}
    };

    int failures = 0;

    std::cout << "BioFlow Studio automated tests\n"
              << "================================\n";

    for (const TestCase& test : tests)
    {
        try
        {
            test.function();
            std::cout << "[PASS] " << test.name << '\n';
        }
        catch (const std::exception& error)
        {
            ++failures;
            std::cout << "[FAIL] " << test.name
                      << "\n       " << error.what() << '\n';
        }
    }

    std::cout << "================================\n"
              << (tests.size() - static_cast<std::size_t>(failures))
              << " passed, " << failures << " failed.\n";

    return failures == 0 ? 0 : 1;
}
