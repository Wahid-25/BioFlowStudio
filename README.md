# BioFlow Studio

A C++17 and Qt desktop application that brings sequence analysis,
gene expression analysis, and workflow management into one interface.

Developed as a team bioinformatics project using object-oriented design.

## Analysis Modules

### Phylogenetic Analysis
- FASTA sequence parsing
- Pairwise alignment and distance matrix calculation
- UPGMA tree construction and visualization
- Export of analysis results

### Gene Expression Analysis
- CSV and TSV expression data parsing
- Data filtering, normalization, and sample grouping
- Differential expression analysis using Welch's t-test
- Principal component analysis (PCA)
- Pathway enrichment analysis
- Heatmap, volcano plot, PCA plot, and enrichment chart visualization

### Workflow and Project Management
- Workflow graph and canvas components
- Sequence and expression data quality checks
- Project session saving and loading
- HTML report generation

## Technologies

- C++17
- Qt6: Core, Widgets, and Charts
- CMake
- Git and GitHub

## Project Structure

| Directory | Purpose |
|-----------|---------|
| `src/core/` | Project and dataset models |
| `src/phylogenetics/` | Sequence alignment, distance matrices, and trees |
| `src/gene_expression/` | Expression processing and statistical analysis |
| `src/workflow/` | Workflow steps and graph management |
| `src/visualization/` | Plot and visualization widgets |
| `src/quality/` | Data quality checks |
| `src/session/` | Project persistence |
| `src/export/` | Result export |
| `src/report/` | HTML reports |
| `ui/` | Main application interface |
| `data/` | Sample datasets and invalid-input examples |
| `tests/` | Core test runner |

## Building on Windows

### Requirements
- CMake 3.21 or newer
- A C++17-compatible compiler
- Qt6 with Core, Widgets, and Charts
- A compiler compatible with the installed Qt build

The commands below assume an MSYS2 UCRT64 installation at
`C:/msys64/ucrt64`, with its compiler and Make tools available in PATH.

### Download the Current Development Branch

```powershell
git clone --branch feature/modern-dashboard-ui https://github.com/Wahid-25/BioFlowStudio.git
cd BioFlowStudio
```

### Configure and Build

```powershell
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH="C:/msys64/ucrt64"
cmake --build build
```

Adjust the Qt installation path if your setup differs.

### Run

```powershell
.\build\BioFlowStudio.exe
```

Qt runtime libraries must be available when launching the application.

### Run Core Tests

```powershell
ctest --test-dir build --output-on-failure
```

## Sample Data

Sample FASTA files are available in `data/phylogenetics/`.

Sample CSV and TSV expression datasets are available in
`data/gene_expression/`.

Some files intentionally contain invalid inputs to exercise validation.

## Project Scope

BioFlow Studio is an educational bioinformatics application.
Analysis results depend on the selected methods, settings, and input data.