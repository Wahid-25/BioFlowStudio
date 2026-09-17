#include "WorkflowTemplateFactory.h"

WorkflowGraph WorkflowTemplateFactory::create(WorkflowTemplateType type)
{
    return type == WorkflowTemplateType::Phylogenetic
        ? createPhylogeneticWorkflow()
        : createExpressionWorkflow();
}

WorkflowGraph WorkflowTemplateFactory::createPhylogeneticWorkflow()
{
    WorkflowGraph graph;

    int input = graph.addNode(
        WorkflowNodeKind::Input,
        "Import FASTA",
        "Load nucleotide or protein sequences from one or more FASTA files.",
        45, 80
    );
    int quality = graph.addNode(
        WorkflowNodeKind::Processing,
        "Sequence Quality",
        "Check identifiers, sequence types, lengths and invalid characters.",
        285, 80
    );
    int distance = graph.addNode(
        WorkflowNodeKind::Processing,
        "Distance Matrix",
        "Calculate pairwise Hamming or Needleman-Wunsch distances.",
        525, 80
    );
    int tree = graph.addNode(
        WorkflowNodeKind::Processing,
        "UPGMA Tree",
        "Cluster sequences hierarchically from the distance matrix.",
        765, 80
    );
    int visualization = graph.addNode(
        WorkflowNodeKind::Visualization,
        "Tree Visualization",
        "Inspect the graphical tree and Newick representation.",
        525, 235
    );
    int output = graph.addNode(
        WorkflowNodeKind::Output,
        "Export and Report",
        "Export CSV, PHYLIP, Newick, PNG and an automatic HTML report.",
        765, 235
    );

    graph.addConnection(input, quality);
    graph.addConnection(quality, distance);
    graph.addConnection(distance, tree);
    graph.addConnection(tree, visualization);
    graph.addConnection(visualization, output);
    return graph;
}

WorkflowGraph WorkflowTemplateFactory::createExpressionWorkflow()
{
    WorkflowGraph graph;

    int input = graph.addNode(
        WorkflowNodeKind::Input,
        "Import Expression Data",
        "Load a CSV or TSV gene-expression matrix.",
        30, 65
    );
    int quality = graph.addNode(
        WorkflowNodeKind::Processing,
        "Data Quality",
        "Evaluate completeness, duplicates, zeros and replicate readiness.",
        270, 65
    );
    int grouping = graph.addNode(
        WorkflowNodeKind::Processing,
        "Groups and Normalization",
        "Assign control/treatment samples and choose normalization.",
        510, 65
    );
    int differential = graph.addNode(
        WorkflowNodeKind::Processing,
        "Differential Expression",
        "Run Welch's t-test and Benjamini-Hochberg correction.",
        750, 65
    );
    int visualization = graph.addNode(
        WorkflowNodeKind::Visualization,
        "Plots and Exploration",
        "Explore the result table, volcano plot, heatmap and PCA.",
        270, 225
    );
    int enrichment = graph.addNode(
        WorkflowNodeKind::Processing,
        "Pathway Enrichment",
        "Interpret significant genes using functional pathways.",
        510, 225
    );
    int output = graph.addNode(
        WorkflowNodeKind::Output,
        "Export and Report",
        "Export result tables, plot images and a complete HTML report.",
        750, 225
    );

    graph.addConnection(input, quality);
    graph.addConnection(quality, grouping);
    graph.addConnection(grouping, differential);
    graph.addConnection(differential, visualization);
    graph.addConnection(differential, enrichment);
    graph.addConnection(visualization, output);
    graph.addConnection(enrichment, output);
    return graph;
}
