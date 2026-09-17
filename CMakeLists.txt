cmake_minimum_required(VERSION 3.21)

project(BioFlowStudio VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Charts)

qt_standard_project_setup()

qt_add_executable(BioFlowStudio
    src/main.cpp
    src/phylogenetics/UPGMATree.cpp
    src/visualization/PhylogeneticTreeWidget.cpp
    src/visualization/PhylogeneticTreeWidget.h
    src/visualization/ExpressionHeatmapWidget.cpp
    src/visualization/ExpressionHeatmapWidget.h
    src/visualization/PCAPlotWidget.cpp
    src/visualization/PCAPlotWidget.h
    src/visualization/VolcanoPlotWidget.cpp
    src/visualization/VolcanoPlotWidget.h
    src/visualization/EnrichmentBarChartWidget.cpp
    src/visualization/EnrichmentBarChartWidget.h
    src/phylogenetics/UPGMATree.h
    src/export/PhylogeneticExporter.cpp
    src/export/PhylogeneticExporter.h
    src/export/GeneExpressionExporter.cpp
    src/export/GeneExpressionExporter.h
    src/phylogenetics/PairwiseAligner.cpp
    src/gene_expression/ExpressionDataset.cpp
    src/gene_expression/ExpressionDataset.h
    src/gene_expression/ExpressionParser.cpp
    src/gene_expression/ExpressionParser.h
    src/phylogenetics/PairwiseAligner.h
    src/phylogenetics/DistanceMatrix.cpp
    src/phylogenetics/DistanceMatrix.h
    src/gene_expression/NormalizationStrategy.cpp
    src/gene_expression/NormalizationStrategy.h
    src/gene_expression/SampleGrouping.cpp
    src/gene_expression/SampleGrouping.h
    src/phylogenetics/Sequence.cpp
    src/phylogenetics/Sequence.h
    src/gene_expression/DifferentialExpressionResult.cpp
    src/gene_expression/DifferentialExpressionResult.h
    src/gene_expression/DifferentialExpressionAnalyzer.h
    src/gene_expression/ExpressionFilter.cpp
    src/gene_expression/ExpressionFilter.h
    src/gene_expression/EnrichmentResult.cpp
    src/gene_expression/EnrichmentResult.h
    src/gene_expression/PathwayDatabase.cpp
    src/gene_expression/PathwayDatabase.h
    src/gene_expression/EnrichmentAnalyzer.h
    src/gene_expression/HypergeometricEnrichmentAnalyzer.cpp
    src/gene_expression/HypergeometricEnrichmentAnalyzer.h
    src/phylogenetics/FastaParser.cpp
    src/phylogenetics/FastaParser.h
    src/gene_expression/StatisticsUtilities.cpp
    src/gene_expression/StatisticsUtilities.h
    src/gene_expression/WelchTTestAnalyzer.cpp
    src/gene_expression/WelchTTestAnalyzer.h
    src/gene_expression/PCAAnalyzer.cpp
    src/gene_expression/PCAAnalyzer.h
    src/gene_expression/PCAResult.cpp
    src/gene_expression/PCAResult.h
    src/core/Project.cpp
    src/core/Project.h
    src/core/Dataset.cpp
    src/core/Dataset.h

    src/quality/QualityReport.cpp
    src/quality/QualityReport.h
    src/quality/DataQualityAnalyzer.h
    src/quality/SequenceQualityAnalyzer.cpp
    src/quality/SequenceQualityAnalyzer.h
    src/quality/ExpressionQualityAnalyzer.cpp
    src/quality/ExpressionQualityAnalyzer.h

    src/workflow/AnalysisContext.cpp
    src/workflow/AnalysisContext.h
    src/workflow/AnalysisResult.cpp
    src/workflow/AnalysisResult.h
    src/workflow/AnalysisStep.h
    src/workflow/Workflow.cpp
    src/workflow/Workflow.h

    ui/MainWindow.cpp
    ui/MainWindow.h
)

target_include_directories(BioFlowStudio PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/workflow
    ${CMAKE_CURRENT_SOURCE_DIR}/ui

    ${CMAKE_CURRENT_SOURCE_DIR}/src/phylogenetics
)

target_link_libraries(BioFlowStudio PRIVATE
    Qt6::Widgets
    Qt6::Charts
)

set_target_properties(BioFlowStudio PROPERTIES
    WIN32_EXECUTABLE TRUE
)
