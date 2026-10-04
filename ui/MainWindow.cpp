#include "MainWindow.h"

#include <QAbstractItemView>
#include <QAction>
#include <QApplication>
#include <QButtonGroup>
#include <QColor>
#include <QCloseEvent>
#include <QComboBox>
#include <QDir>
#include <QDateTime>
#include <QDesktopServices>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QGridLayout>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QIcon>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPixmap>
#include <QPainter>
#include <QPainterPath>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSizePolicy>
#include <QStackedWidget>
#include <QStatusBar>
#include <QStringList>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <QWidget>
#include <QUrl>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>

#include <algorithm>
#include <exception>
#include <fstream>
#include <iomanip>
#include <memory>
#include <stdexcept>
#include <utility>

#include "export/PhylogeneticExporter.h"
#include "export/GeneExpressionExporter.h"
#include "gene_expression/NormalizationStrategy.h"
#include "gene_expression/PCAAnalyzer.h"
#include "gene_expression/ExpressionParser.h"
#include "gene_expression/HypergeometricEnrichmentAnalyzer.h"
#include "gene_expression/PathwayDatabase.h"
#include "gene_expression/WelchTTestAnalyzer.h"
#include "phylogenetics/FastaParser.h"
#include "phylogenetics/PairwiseAligner.h"
#include "quality/ExpressionQualityAnalyzer.h"
#include "quality/SequenceQualityAnalyzer.h"
#include "report/HtmlReportGenerator.h"
#include "session/ProjectSerializer.h"
#include "visualization/PhylogeneticTreeWidget.h"
#include "visualization/ExpressionHeatmapWidget.h"
#include "visualization/EnrichmentBarChartWidget.h"
#include "visualization/PCAPlotWidget.h"
#include "visualization/VolcanoPlotWidget.h"
#include "visualization/WorkflowCanvasWidget.h"
#include "workflow/WorkflowTemplateFactory.h"

namespace
{
class NumericTableWidgetItem : public QTableWidgetItem
{
public:
    NumericTableWidgetItem(double value, int precision)
        : QTableWidgetItem(QString::number(value, 'g', precision)),
          numericValue(value)
    {
        setTextAlignment(Qt::AlignCenter);
    }

    bool operator<(const QTableWidgetItem& other) const override
    {
        const auto* numericOther =
            dynamic_cast<const NumericTableWidgetItem*>(&other);

        if (numericOther != nullptr)
        {
            return numericValue < numericOther->numericValue;
        }

        return QTableWidgetItem::operator<(other);
    }

private:
    double numericValue;
};

QIcon createBioFlowIcon()
{
    QPixmap pixmap(64, 64);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor("#163A5F"));
    painter.drawRoundedRect(2, 2, 60, 60, 13, 13);

    QPainterPath leftStrand;
    leftStrand.moveTo(20, 12);
    leftStrand.cubicTo(46, 23, 18, 41, 44, 52);

    QPainterPath rightStrand;
    rightStrand.moveTo(44, 12);
    rightStrand.cubicTo(18, 23, 46, 41, 20, 52);

    QPen strandPen(QColor("#62D6C5"), 4.0, Qt::SolidLine, Qt::RoundCap);
    painter.setPen(strandPen);
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(leftStrand);
    painter.drawPath(rightStrand);

    QPen rungPen(QColor("#FFFFFF"), 2.4, Qt::SolidLine, Qt::RoundCap);
    painter.setPen(rungPen);
    painter.drawLine(25, 18, 39, 18);
    painter.drawLine(22, 27, 42, 27);
    painter.drawLine(22, 37, 42, 37);
    painter.drawLine(25, 46, 39, 46);

    return QIcon(pixmap);
}

QFrame* createDashboardStatCard(
    const QString& value,
    const QString& label,
    const QString& accent
)
{
    QFrame* card = new QFrame;
    card->setObjectName("statCard");
    card->setProperty("accent", accent);
    card->setFixedHeight(64);

    QVBoxLayout* layout = new QVBoxLayout(card);
    layout->setContentsMargins(15, 9, 15, 9);
    layout->setSpacing(0);

    QLabel* valueLabel = new QLabel(value);
    valueLabel->setObjectName("statValue");

    QLabel* nameLabel = new QLabel(label);
    nameLabel->setObjectName("statName");

    layout->addWidget(valueLabel);
    layout->addWidget(nameLabel);

    return card;
}

QFrame* createWorkspaceCard(
    const QString& eyebrow,
    const QString& title,
    const QString& description,
    QPushButton*& actionButton
)
{
    QFrame* card = new QFrame;
    card->setObjectName("workspaceCard");
    card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    card->setMinimumHeight(205);
    card->setMaximumHeight(225);

    QVBoxLayout* layout = new QVBoxLayout(card);
    layout->setContentsMargins(18, 16, 18, 16);
    layout->setSpacing(7);

    QLabel* eyebrowLabel = new QLabel(eyebrow.toUpper());
    eyebrowLabel->setObjectName("cardEyebrow");

    QLabel* titleLabel = new QLabel(title);
    titleLabel->setObjectName("cardTitle");

    QLabel* descriptionLabel = new QLabel(description);
    descriptionLabel->setObjectName("cardDescription");
    descriptionLabel->setWordWrap(true);

    actionButton = new QPushButton("Open  →");
    actionButton->setProperty("primary", true);
    actionButton->setCursor(Qt::PointingHandCursor);

    layout->addWidget(eyebrowLabel);
    layout->addWidget(titleLabel);
    layout->addWidget(descriptionLabel);
    layout->addStretch();
    layout->addWidget(actionButton, 0, Qt::AlignLeft);

    return card;
}

void styleDashboardTable(QTableWidget* table)
{
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setAlternatingRowColors(true);
    table->verticalHeader()->setVisible(false);
    table->setStyleSheet(
        "QHeaderView { background-color: #0F172A; }"
        "QHeaderView::section {"
        " background-color: #0F172A; color: #9FB2CE;"
        " border: 1px solid #334155; padding: 7px; font-weight: 600;"
        "}"
        "QTableCornerButton::section {"
        " background-color: #0F172A; border: 1px solid #334155;"
        "}"
    );
}

void fitEmbeddedHeatmapTable(QWidget* widget)
{
    QTableWidget* table = qobject_cast<QTableWidget*>(widget);

    if (table == nullptr)
    {
        table = widget->findChild<QTableWidget*>();
    }

    if (table == nullptr)
    {
        return;
    }

    table->setCornerButtonEnabled(false);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->horizontalHeader()->setStretchLastSection(true);
    table->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
}
}

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      currentProject(
          "BioFlow Demonstration",
          "A workflow platform for biological data analysis",
          WorkspaceType::NotSelected
      )
{
    QIcon applicationIcon = createBioFlowIcon();
    QApplication::setWindowIcon(applicationIcon);
    setWindowIcon(applicationIcon);
    updateWindowTitle();
    resize(1100, 700);
    setMinimumSize(900, 600);

    pages = new QStackedWidget;
    pages->setObjectName("pageStack");

    pages->addWidget(createDashboardPage());
    pages->addWidget(createPhylogeneticSetupPage());
    pages->addWidget(createDistanceMatrixPage());
    pages->addWidget(createPhylogeneticTreePage());
    pages->addWidget(createGeneExpressionPage());
    pages->addWidget(createExpressionConfigurationPage());
    pages->addWidget(createExpressionResultsPage());
    pages->addWidget(createExpressionVolcanoPage());
    pages->addWidget(createExpressionHeatmapPage());
    pages->addWidget(createExpressionPCAPage());
    pages->addWidget(createPhylogeneticQualityPage());
    pages->addWidget(createExpressionQualityPage());
    pages->addWidget(createExpressionEnrichmentPage());
    pages->addWidget(createWorkflowBuilderPage());
    pages->addWidget(createProjectHistoryPage());
    pages->addWidget(createPhylogeneticHeatmapPage());

    pages->setCurrentIndex(DashboardPage);
    setCentralWidget(createApplicationShell());

    QMenu* fileMenu = menuBar()->addMenu("File");
    QAction* openProjectAction = fileMenu->addAction("Open Project...");
    QAction* saveProjectAction = fileMenu->addAction("Save Project...");
    fileMenu->addSeparator();
    QAction* exitAction = fileMenu->addAction("Exit");

    QMenu* viewMenu = menuBar()->addMenu("View");
    QAction* historyAction = viewMenu->addAction("Analysis History");

    QMenu* analysisMenu = menuBar()->addMenu("Analysis");
    QAction* phylogeneticAction =
        analysisMenu->addAction("Phylogenetic Analysis");
    QAction* expressionAction =
        analysisMenu->addAction("Gene Expression Analysis");
    analysisMenu->addSeparator();
    QAction* workflowAction =
        analysisMenu->addAction("Workflow Builder");

    QMenu* helpMenu = menuBar()->addMenu("Help");
    QAction* aboutAction = helpMenu->addAction("About BioFlow Studio");

    openProjectAction->setShortcut(QKeySequence::Open);
    saveProjectAction->setShortcut(QKeySequence::Save);
    exitAction->setShortcut(QKeySequence::Quit);
    historyAction->setShortcut(QKeySequence("Ctrl+H"));
    aboutAction->setShortcut(QKeySequence("F1"));

    openProjectAction->setStatusTip("Open a saved BioFlow project");
    saveProjectAction->setStatusTip("Save datasets, settings and history");
    historyAction->setStatusTip("View the current project's analysis record");
    workflowAction->setStatusTip("Open the graphical workflow builder");

    connect(
        openProjectAction,
        &QAction::triggered,
        this,
        &MainWindow::loadProject
    );
    connect(
        saveProjectAction,
        &QAction::triggered,
        this,
        &MainWindow::saveProject
    );
    connect(
        historyAction,
        &QAction::triggered,
        this,
        &MainWindow::openProjectHistoryPage
    );
    connect(
        phylogeneticAction,
        &QAction::triggered,
        this,
        &MainWindow::selectPhylogeneticWorkspace
    );
    connect(
        expressionAction,
        &QAction::triggered,
        this,
        &MainWindow::selectGeneExpressionWorkspace
    );
    connect(
        workflowAction,
        &QAction::triggered,
        this,
        &MainWindow::openWorkflowBuilderPage
    );
    connect(
        aboutAction,
        &QAction::triggered,
        this,
        &MainWindow::showAboutDialog
    );
    connect(exitAction, &QAction::triggered, this, &QWidget::close);

    connect(
        pages,
        &QStackedWidget::currentChanged,
        this,
        &MainWindow::updateApplicationShell
    );

    updateApplicationShell(DashboardPage);

    statusBar()->showMessage(
        "Ready | Open a workspace or load a saved BioFlow project"
    );

    const std::vector<QComboBox*> savedSettingBoxes = {
        matrixAlignmentMethodBox,
        treeAlignmentMethodBox,
        normalizationMethodBox,
        workflowTemplateBox
    };

    for (QComboBox* box : savedSettingBoxes)
    {
        connect(
            box,
            &QComboBox::currentIndexChanged,
            this,
            [this](int)
            {
                markProjectModified();
            }
        );
    }

    const std::vector<QDoubleSpinBox*> savedThresholdBoxes = {
        adjustedPThresholdBox,
        foldChangeThresholdBox,
        enrichmentPThresholdBox
    };

    for (QDoubleSpinBox* box : savedThresholdBoxes)
    {
        connect(
            box,
            &QDoubleSpinBox::valueChanged,
            this,
            [this](double)
            {
                markProjectModified();
            }
        );
    }

    projectModified = false;
    updateWindowTitle();

    setStyleSheet(
        R"(
        QWidget {
            color: #F8FAFC;
            font-family: "Outfit", "Segoe UI";
            font-size: 14px;
        }

        QMainWindow,
        QWidget#appRoot,
        QStackedWidget#pageStack {
            background-color: #0F172A;
        }

        QMenuBar {
            background-color: #0F172A;
            color: #94A3B8;
            border-bottom: 1px solid #334155;
            spacing: 4px;
        }

        QMenuBar::item {
            background-color: transparent;
            padding: 7px 14px;
            margin: 0 2px;
        }

        QMenuBar::item:selected,
        QMenuBar::item:pressed {
            background-color: #1E293B;
            color: #2563EB;
            border-radius: 6px;
        }

        QMenu {
            background-color: #1E293B;
            color: #F8FAFC;
            border: 1px solid #334155;
            padding: 6px;
            min-width: 260px;
        }

        QMenu::item {
            background-color: transparent;
            padding: 8px 58px 8px 14px;
            min-width: 188px;
        }

        QMenu::item:selected {
            background-color: #334155;
            color: #F8FAFC;
            border-radius: 6px;
        }

        QMenu::separator {
            height: 1px;
            background-color: #334155;
            margin: 5px 10px;
        }

        #titleLabel {
            color: #F8FAFC;
            font-size: 30px;
            font-weight: 700;
        }

        #subtitleLabel {
            color: #94A3B8;
            font-size: 15px;
        }

        #descriptionLabel {
            color: #94A3B8;
            font-size: 14px;
        }

        #statusLabel {
            background-color: #064E3B;
            color: #6EE7B7;
            border: 1px solid #047857;
            border-radius: 10px;
            font-size: 13px;
            font-weight: 600;
            padding: 8px 12px;
        }

        QFrame#appSidebar {
            background-color: #0F172A;
            border-right: 1px solid #334155;
        }

        QFrame#applicationHeader {
            background-color: #0F172A;
            border-bottom: 1px solid #334155;
        }

        QLabel#shellTitle {
            color: #F8FAFC;
            font-size: 19px;
            font-weight: 700;
        }

        QLabel#shellSubtitle {
            color: #94A3B8;
            font-size: 12px;
        }

        QLabel#brandText {
            color: #F8FAFC;
            font-size: 12px;
            font-weight: 700;
        }

        QPushButton#sidebarNavButton {
            background-color: transparent;
            color: #94A3B8;
            border: none;
            border-radius: 12px;
            min-width: 48px;
            max-width: 48px;
            min-height: 44px;
            max-height: 44px;
            padding: 0;
            font-size: 12px;
            font-weight: 700;
        }

        QPushButton#sidebarNavButton:hover {
            background-color: #1E293B;
            color: #60A5FA;
        }

        QPushButton#sidebarNavButton:checked {
            background-color: #1E3A5F;
            color: #60A5FA;
        }

        QPushButton {
            background-color: #1E293B;
            color: #CBD5E1;
            border: 1px solid #334155;
            border-radius: 10px;
            font-size: 14px;
            font-weight: 600;
            padding: 9px 14px;
        }

        QPushButton:hover {
            background-color: #273449;
            border-color: #475569;
            color: #60A5FA;
        }

        QPushButton:pressed {
            background-color: #1E3A5F;
        }

        QPushButton[primary="true"] {
            background-color: #2563EB;
            color: #FFFFFF;
            border-color: #2563EB;
        }

        QPushButton[primary="true"]:hover {
            background-color: #1D4ED8;
            border-color: #1D4ED8;
            color: #FFFFFF;
        }

        QPushButton:disabled {
            background-color: #334155;
            color: #64748B;
        }

        QPushButton#compactActionButton {
            background-color: #1E293B;
            color: #CBD5E1;
            border: 1px solid #334155;
            border-radius: 5px;
            font-size: 13px;
            font-weight: bold;
            padding: 6px 12px;
        }

        QPushButton#compactActionButton:hover {
            background-color: #273449;
            border-color: #475569;
        }

        QPushButton#compactPrimaryButton {
            background-color: #2563EB;
            color: white;
            border: 1px solid #2563EB;
            border-radius: 5px;
            font-size: 13px;
            font-weight: bold;
            padding: 6px 13px;
        }

        QPushButton#compactPrimaryButton:hover {
            background-color: #1D4ED8;
        }

        QPushButton#compactNavigationButton {
            background-color: transparent;
            color: #CBD5E1;
            border: 1px solid #334155;
            border-radius: 5px;
            font-size: 13px;
            font-weight: bold;
            padding: 6px 12px;
        }

        QPushButton#compactNavigationButton:hover {
            background-color: #1E293B;
            color: #60A5FA;
        }

        QFrame#statCard,
        QFrame#workspaceCard {
            background-color: #1E293B;
            border: 1px solid #334155;
            border-radius: 16px;
        }

        QFrame#workspaceCard:hover {
            border-color: #3B82F6;
        }

        QLabel#statValue {
            color: #F8FAFC;
            font-size: 20px;
            font-weight: 700;
        }

        QLabel#statName {
            color: #94A3B8;
            font-size: 11px;
        }

        QLabel#cardEyebrow {
            color: #2563EB;
            font-size: 11px;
            font-weight: 700;
        }

        QLabel#cardTitle {
            color: #F8FAFC;
            font-size: 17px;
            font-weight: 700;
        }

        QLabel#cardDescription {
            color: #94A3B8;
            font-size: 12px;
        }

        QFrame#analysisCard {
            background-color: #1E293B;
            border: 1px solid #334155;
            border-radius: 16px;
        }

        QLabel#panelTitle {
            color: #F8FAFC;
            font-size: 18px;
            font-weight: 700;
        }

        QLabel#panelDescription {
            color: #94A3B8;
            font-size: 12px;
        }

        QLabel#panelHint {
            color: #64748B;
            font-size: 11px;
        }

        QLabel#fieldLabel {
            color: #CBD5E1;
            font-size: 12px;
            font-weight: 600;
        }

        QLabel#resultStatusBadge {
            background-color: #0F172A;
            color: #94A3B8;
            border: 1px solid #334155;
            border-radius: 9px;
            padding: 6px 10px;
            font-size: 11px;
            font-weight: 600;
        }

        QPushButton#analysisOptionButton {
            min-height: 48px;
            padding: 10px 14px;
            text-align: left;
            background-color: #0F172A;
            color: #CBD5E1;
            border: 1px solid #334155;
            border-radius: 10px;
        }

        QPushButton#analysisOptionButton:hover {
            background-color: #1E3A5F;
            color: #F8FAFC;
            border-color: #3B82F6;
        }

        QPushButton#analysisOptionButton:disabled {
            background-color: #172033;
            color: #64748B;
            border-color: #334155;
        }

        QListWidget {
            background-color: #1E293B;
            color: #F8FAFC;
            border: 1px solid #334155;
            border-radius: 12px;
            padding: 8px;
        }

        QListWidget::item:selected,
        QTableWidget::item:selected {
            background-color: #1D4ED8;
            color: #FFFFFF;
        }

        QLineEdit,
        QComboBox,
        QDoubleSpinBox {
            background-color: #1E293B;
            color: #F8FAFC;
            border: 1px solid #334155;
            border-radius: 10px;
            padding: 9px;
        }

        QLineEdit:focus,
        QComboBox:focus,
        QDoubleSpinBox:focus {
            border-color: #2563EB;
        }

        QComboBox QAbstractItemView {
            background-color: #1E293B;
            color: #F8FAFC;
            border: 1px solid #334155;
            selection-background-color: #1D4ED8;
        }

        QTableWidget {
            background-color: #1E293B;
            alternate-background-color: #172033;
            color: #F8FAFC;
            border: 1px solid #334155;
            border-radius: 12px;
            gridline-color: #334155;
        }

        QHeaderView::section {
            background-color: #0F172A;
            color: #94A3B8;
            font-weight: bold;
            padding: 6px;
            border: none;
            border-bottom: 1px solid #334155;
        }

        QPlainTextEdit {
            background-color: #1E293B;
            color: #F8FAFC;
            border: 1px solid #334155;
            border-radius: 12px;
            font-family: Consolas;
            font-size: 13px;
        }

        QToolTip {
            background-color: #1E293B;
            color: #F8FAFC;
            border: 1px solid #334155;
            padding: 5px;
        }

        QStatusBar {
            background-color: #0F172A;
            color: #94A3B8;
            border-top: 1px solid #334155;
        }

        QStatusBar::item {
            border: none;
        }
        )"
    );
}

QWidget* MainWindow::createApplicationShell()
{
    QWidget* shell = new QWidget;
    shell->setObjectName("appRoot");

    QHBoxLayout* shellLayout = new QHBoxLayout(shell);
    shellLayout->setContentsMargins(0, 0, 0, 0);
    shellLayout->setSpacing(0);

    QFrame* sidebar = new QFrame;
    sidebar->setObjectName("appSidebar");
    sidebar->setFixedWidth(85);

    QVBoxLayout* sidebarLayout = new QVBoxLayout(sidebar);
    sidebarLayout->setContentsMargins(12, 14, 12, 14);
    sidebarLayout->setSpacing(8);
    sidebarLayout->setAlignment(Qt::AlignHCenter);

    QLabel* logoLabel = new QLabel;
    logoLabel->setPixmap(createBioFlowIcon().pixmap(44, 44));
    logoLabel->setAlignment(Qt::AlignCenter);

    QLabel* brandLabel = new QLabel("BIOFLOW");
    brandLabel->setObjectName("brandText");
    brandLabel->setAlignment(Qt::AlignCenter);

    sidebarLayout->addWidget(logoLabel);
    sidebarLayout->addWidget(brandLabel);
    sidebarLayout->addSpacing(12);

    navigationGroup = new QButtonGroup(this);
    navigationGroup->setExclusive(true);

    auto addNavigationButton = [this, sidebarLayout](
        const QString& text,
        const QString& toolTip,
        int navigationId
    )
    {
        QPushButton* button = new QPushButton(text);
        button->setObjectName("sidebarNavButton");
        button->setToolTip(toolTip);
        button->setCheckable(true);
        button->setCursor(Qt::PointingHandCursor);
        navigationGroup->addButton(button, navigationId);
        sidebarLayout->addWidget(button, 0, Qt::AlignHCenter);
    };

    addNavigationButton("HOME", "Dashboard", 0);
    addNavigationButton("DNA", "Phylogenetic Analysis", 1);
    addNavigationButton("GE", "Gene Expression Analysis", 2);
    addNavigationButton("FLOW", "Workflow Builder", 3);
    addNavigationButton("LOG", "Project History", 4);

    sidebarLayout->addStretch();

    QPushButton* helpButton = new QPushButton("?");
    helpButton->setObjectName("sidebarNavButton");
    helpButton->setToolTip("About BioFlow Studio");
    helpButton->setCursor(Qt::PointingHandCursor);
    sidebarLayout->addWidget(helpButton, 0, Qt::AlignHCenter);

    connect(
        helpButton,
        &QPushButton::clicked,
        this,
        &MainWindow::showAboutDialog
    );

    connect(
        navigationGroup,
        &QButtonGroup::idClicked,
        this,
        [this](int navigationId)
        {
            switch (navigationId)
            {
                case 0:
                    returnToDashboard();
                    break;
                case 1:
                    selectPhylogeneticWorkspace();
                    break;
                case 2:
                    selectGeneExpressionWorkspace();
                    break;
                case 3:
                    openWorkflowBuilderPage();
                    break;
                case 4:
                    openProjectHistoryPage();
                    break;
                default:
                    break;
            }
        }
    );

    QWidget* contentArea = new QWidget;
    QVBoxLayout* contentLayout = new QVBoxLayout(contentArea);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(0);

    QFrame* header = new QFrame;
    header->setObjectName("applicationHeader");
    header->setMinimumHeight(72);

    QHBoxLayout* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(24, 10, 20, 10);
    headerLayout->setSpacing(12);

    QVBoxLayout* titleLayout = new QVBoxLayout;
    titleLayout->setContentsMargins(0, 0, 0, 0);
    titleLayout->setSpacing(2);

    shellPageTitleLabel = new QLabel("Dashboard");
    shellPageTitleLabel->setObjectName("shellTitle");

    shellPageSubtitleLabel = new QLabel("Your bioinformatics workspace");
    shellPageSubtitleLabel->setObjectName("shellSubtitle");

    titleLayout->addWidget(shellPageTitleLabel);
    titleLayout->addWidget(shellPageSubtitleLabel);

    shellBackButton = new QPushButton("←  Back");
    shellBackButton->setObjectName("compactNavigationButton");
    shellBackButton->setCursor(Qt::PointingHandCursor);
    shellBackButton->setToolTip("Return to the previous analysis screen");

    QPushButton* openButton = new QPushButton("Open project");
    openButton->setObjectName("compactActionButton");
    openButton->setCursor(Qt::PointingHandCursor);

    QPushButton* saveButton = new QPushButton("Save project");
    saveButton->setObjectName("compactPrimaryButton");
    saveButton->setCursor(Qt::PointingHandCursor);

    connect(openButton, &QPushButton::clicked, this, &MainWindow::loadProject);
    connect(saveButton, &QPushButton::clicked, this, &MainWindow::saveProject);

    connect(
        shellBackButton,
        &QPushButton::clicked,
        this,
        [this]()
        {
            switch (pages->currentIndex())
            {
                case PhylogeneticSetupPage:
                case GeneExpressionPage:
                case WorkflowBuilderPage:
                case ProjectHistoryPage:
                    returnToDashboard();
                    break;
                case PhylogeneticHeatmapPage:
                    openDistanceMatrixPage();
                    break;
                case DistanceMatrixPage:
                case PhylogeneticTreePage:
                case PhylogeneticQualityPage:
                    returnToPhylogeneticSetup();
                    break;
                case ExpressionConfigurationPage:
                case ExpressionQualityPage:
                    returnToGeneExpressionSetup();
                    break;
                case ExpressionResultsPage:
                    openExpressionConfigurationPage();
                    break;
                case ExpressionVolcanoPage:
                case ExpressionHeatmapPage:
                case ExpressionPCAPage:
                case ExpressionEnrichmentPage:
                    returnToExpressionResults();
                    break;
                default:
                    returnToDashboard();
                    break;
            }
        }
    );

    headerLayout->addWidget(shellBackButton);
    headerLayout->addLayout(titleLayout);
    headerLayout->addStretch();
    headerLayout->addWidget(openButton);
    headerLayout->addWidget(saveButton);

    contentLayout->addWidget(header);
    contentLayout->addWidget(pages, 1);

    shellLayout->addWidget(sidebar);
    shellLayout->addWidget(contentArea, 1);

    return shell;
}

void MainWindow::updateApplicationShell(int pageIndex)
{
    QString pageTitle = "Dashboard";
    QString pageSubtitle = "Your bioinformatics workspace";
    int navigationId = 0;

    switch (pageIndex)
    {
        case PhylogeneticSetupPage:
            pageTitle = "Phylogenetic Analysis";
            pageSubtitle = "Import and validate biological sequences";
            navigationId = 1;
            break;
        case DistanceMatrixPage:
            pageTitle = "Distance Matrix";
            pageSubtitle = "Compare pairwise evolutionary distances";
            navigationId = 1;
            break;
        case PhylogeneticHeatmapPage:
            pageTitle = "Distance Heatmap";
            pageSubtitle = "Explore pairwise distance patterns";
            navigationId = 1;
            break;
        case PhylogeneticTreePage:
            pageTitle = "Phylogenetic Tree";
            pageSubtitle = "Explore the generated UPGMA tree";
            navigationId = 1;
            break;
        case PhylogeneticQualityPage:
            pageTitle = "Sequence Quality Control";
            pageSubtitle = "Review sequence validity and warnings";
            navigationId = 1;
            break;
        case GeneExpressionPage:
            pageTitle = "Gene Expression";
            pageSubtitle = "Import an expression matrix for analysis";
            navigationId = 2;
            break;
        case ExpressionConfigurationPage:
            pageTitle = "Analysis Configuration";
            pageSubtitle = "Assign sample groups and normalization settings";
            navigationId = 2;
            break;
        case ExpressionResultsPage:
            pageTitle = "Differential Expression";
            pageSubtitle = "Filter and inspect significant genes";
            navigationId = 2;
            break;
        case ExpressionVolcanoPage:
            pageTitle = "Volcano Plot";
            pageSubtitle = "Explore significance and fold change";
            navigationId = 2;
            break;
        case ExpressionHeatmapPage:
            pageTitle = "Expression Heatmap";
            pageSubtitle = "Compare expression patterns across samples";
            navigationId = 2;
            break;
        case ExpressionPCAPage:
            pageTitle = "Principal Component Analysis";
            pageSubtitle = "Inspect sample-level variation and clustering";
            navigationId = 2;
            break;
        case ExpressionQualityPage:
            pageTitle = "Expression Quality Control";
            pageSubtitle = "Review missing values and dataset integrity";
            navigationId = 2;
            break;
        case ExpressionEnrichmentPage:
            pageTitle = "Functional Enrichment";
            pageSubtitle = "Explore pathways associated with significant genes";
            navigationId = 2;
            break;
        case WorkflowBuilderPage:
            pageTitle = "Workflow Builder";
            pageSubtitle = "Design and validate reusable analysis pipelines";
            navigationId = 3;
            break;
        case ProjectHistoryPage:
            pageTitle = "Project History";
            pageSubtitle = "Review the analysis activity recorded for this project";
            navigationId = 4;
            break;
        default:
            break;
    }

    if (shellPageTitleLabel != nullptr)
    {
        shellPageTitleLabel->setText(pageTitle);
    }

    if (shellPageSubtitleLabel != nullptr)
    {
        shellPageSubtitleLabel->setText(pageSubtitle);
    }

    if (shellBackButton != nullptr)
    {
        shellBackButton->setVisible(pageIndex != DashboardPage);
    }

    if (navigationGroup != nullptr && navigationGroup->button(navigationId) != nullptr)
    {
        navigationGroup->button(navigationId)->setChecked(true);
    }
}

QWidget* MainWindow::createDashboardPage()
{
    QWidget* page = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(page);

    layout->setContentsMargins(28, 20, 28, 18);
    layout->setSpacing(12);

    QLabel* title = new QLabel("BioFlow Studio");
    title->setObjectName("titleLabel");
    title->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    QLabel* subtitle = new QLabel(
        "Integrated biological analysis, visualization and reproducible workflow design."
    );

    subtitle->setObjectName("subtitleLabel");
    subtitle->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    QHBoxLayout* statsLayout = new QHBoxLayout;
    statsLayout->setSpacing(12);
    statsLayout->addWidget(createDashboardStatCard("3", "Analysis workspaces", "blue"));
    statsLayout->addWidget(createDashboardStatCard("15", "Integrated views", "green"));
    statsLayout->addWidget(createDashboardStatCard("2", "Supported data types", "purple"));
    statsLayout->addWidget(createDashboardStatCard("Local", "Private computation", "slate"));

    QPushButton* phylogeneticButton = nullptr;
    QPushButton* expressionButton = nullptr;
    QPushButton* workflowButton = nullptr;

    QFrame* phylogeneticCard = createWorkspaceCard(
        "Sequences",
        "Phylogenetic Analysis",
        "Import FASTA sequences, run quality checks, calculate pairwise distances and build a UPGMA tree.",
        phylogeneticButton
    );

    QFrame* expressionCard = createWorkspaceCard(
        "Expression",
        "Gene Expression Analysis",
        "Compare sample groups and explore differential expression using tables, volcano plots, heatmaps and PCA.",
        expressionButton
    );

    QFrame* workflowCard = createWorkspaceCard(
        "Automation",
        "Workflow Builder",
        "Design and validate reusable analysis pipelines through an interactive node-based workflow.",
        workflowButton
    );

    QHBoxLayout* workspaceLayout = new QHBoxLayout;
    workspaceLayout->setSpacing(14);
    workspaceLayout->addWidget(phylogeneticCard, 1);
    workspaceLayout->addWidget(expressionCard, 1);
    workspaceLayout->addWidget(workflowCard, 1);

    statusLabel = new QLabel("No workspace selected");
    statusLabel->setObjectName("statusLabel");
    statusLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    layout->addWidget(title);
    layout->addWidget(subtitle);
    layout->addSpacing(2);
    layout->addLayout(statsLayout);
    layout->addLayout(workspaceLayout, 1);
    layout->addWidget(statusLabel, 0, Qt::AlignLeft);

    connect(
        phylogeneticButton,
        &QPushButton::clicked,
        this,
        &MainWindow::selectPhylogeneticWorkspace
    );

    connect(
        expressionButton,
        &QPushButton::clicked,
        this,
        &MainWindow::selectGeneExpressionWorkspace
    );

    connect(
        workflowButton,
        &QPushButton::clicked,
        this,
        &MainWindow::openWorkflowBuilderPage
    );

    return page;
}

QWidget* MainWindow::createPhylogeneticSetupPage()
{
    QWidget* page = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(page);

    layout->setContentsMargins(28, 22, 28, 24);
    layout->setSpacing(14);

    QHBoxLayout* panelLayout = new QHBoxLayout;
    panelLayout->setSpacing(16);

    QFrame* datasetCard = new QFrame;
    datasetCard->setObjectName("analysisCard");

    QVBoxLayout* datasetLayout = new QVBoxLayout(datasetCard);
    datasetLayout->setContentsMargins(20, 18, 20, 18);
    datasetLayout->setSpacing(10);

    QHBoxLayout* datasetHeader = new QHBoxLayout;

    QVBoxLayout* datasetTitleLayout = new QVBoxLayout;
    datasetTitleLayout->setSpacing(2);

    QLabel* datasetTitle = new QLabel("Sequence dataset");
    datasetTitle->setObjectName("panelTitle");

    QLabel* datasetDescription = new QLabel(
        "Import FASTA sequences and review every validated record."
    );
    datasetDescription->setObjectName("panelDescription");

    datasetTitleLayout->addWidget(datasetTitle);
    datasetTitleLayout->addWidget(datasetDescription);

    QPushButton* importButton = new QPushButton("Import FASTA files");
    importButton->setProperty("primary", true);
    importButton->setCursor(Qt::PointingHandCursor);

    datasetHeader->addLayout(datasetTitleLayout);
    datasetHeader->addStretch();
    datasetHeader->addWidget(importButton);

    phylogeneticFileList = new QListWidget;
    phylogeneticFileList->setMinimumHeight(280);
    phylogeneticFileList->addItem(
        "No FASTA sequences imported."
    );

    QLabel* fileHint = new QLabel(
        "Supported formats: .fasta, .fa, .fna and .faa"
    );
    fileHint->setObjectName("panelHint");

    datasetLayout->addLayout(datasetHeader);
    datasetLayout->addWidget(phylogeneticFileList, 1);
    datasetLayout->addWidget(fileHint);

    QFrame* viewsCard = new QFrame;
    viewsCard->setObjectName("analysisCard");
    viewsCard->setMinimumWidth(330);
    viewsCard->setMaximumWidth(420);

    QVBoxLayout* viewsLayout = new QVBoxLayout(viewsCard);
    viewsLayout->setContentsMargins(20, 18, 20, 18);
    viewsLayout->setSpacing(10);

    QLabel* viewsTitle = new QLabel("Analysis views");
    viewsTitle->setObjectName("panelTitle");

    QLabel* viewsDescription = new QLabel(
        "Import valid sequences to unlock these analysis views."
    );
    viewsDescription->setObjectName("panelDescription");
    viewsDescription->setWordWrap(true);

    viewsLayout->addWidget(viewsTitle);
    viewsLayout->addWidget(viewsDescription);
    viewsLayout->addSpacing(6);

    openMatrixButton =
        new QPushButton(
            "Distance matrix and heatmap  →"
        );

    openTreeButton =
        new QPushButton(
            "UPGMA phylogenetic tree  →"
        );

    phylogeneticQualityButton =
        new QPushButton(
            "Sequence quality control  →"
        );

    openMatrixButton->setObjectName("analysisOptionButton");
    openTreeButton->setObjectName("analysisOptionButton");
    phylogeneticQualityButton->setObjectName("analysisOptionButton");

    openMatrixButton->setCursor(Qt::PointingHandCursor);
    openTreeButton->setCursor(Qt::PointingHandCursor);
    phylogeneticQualityButton->setCursor(Qt::PointingHandCursor);

    openMatrixButton->setEnabled(false);
    openTreeButton->setEnabled(false);
    phylogeneticQualityButton->setEnabled(false);

    viewsLayout->addWidget(phylogeneticQualityButton);
    viewsLayout->addWidget(openMatrixButton);
    viewsLayout->addWidget(openTreeButton);
    viewsLayout->addStretch();

    QLabel* privacyHint = new QLabel(
        "Analysis runs locally. Your sequences are not uploaded."
    );
    privacyHint->setObjectName("panelHint");
    privacyHint->setWordWrap(true);
    viewsLayout->addWidget(privacyHint);

    panelLayout->addWidget(datasetCard, 3);
    panelLayout->addWidget(viewsCard, 2);
    layout->addLayout(panelLayout, 1);

    connect(
        importButton,
        &QPushButton::clicked,
        this,
        &MainWindow::importFastaFiles
    );

    connect(
        openMatrixButton,
        &QPushButton::clicked,
        this,
        &MainWindow::openDistanceMatrixPage
    );

    connect(
        openTreeButton,
        &QPushButton::clicked,
        this,
        &MainWindow::openPhylogeneticTreePage
    );

    connect(
        phylogeneticQualityButton,
        &QPushButton::clicked,
        this,
        &MainWindow::openPhylogeneticQualityPage
    );

    return page;
}

QWidget* MainWindow::createDistanceMatrixPage()
{
    QWidget* page = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(page);

    layout->setContentsMargins(28, 16, 28, 18);
    layout->setSpacing(10);

    QFrame* controlCard = new QFrame;
    controlCard->setObjectName("analysisCard");

    QVBoxLayout* controlLayout = new QVBoxLayout(controlCard);
    controlLayout->setContentsMargins(18, 12, 18, 12);
    controlLayout->setSpacing(7);

    QLabel* controlTitle = new QLabel("Matrix controls");
    controlTitle->setObjectName("panelTitle");

    QLabel* controlDescription = new QLabel(
        "Choose a pairwise strategy, calculate normalized distances and export reproducible results."
    );
    controlDescription->setObjectName("panelDescription");

    QHBoxLayout* controlRow = new QHBoxLayout;
    controlRow->setSpacing(10);

    QLabel* methodLabel = new QLabel("Distance method");
    methodLabel->setObjectName("fieldLabel");

    matrixAlignmentMethodBox = new QComboBox;
    matrixAlignmentMethodBox->setMinimumWidth(270);

    matrixAlignmentMethodBox->addItem(
        "Needleman-Wunsch Global Distance"
    );

    matrixAlignmentMethodBox->addItem(
        "Hamming Distance"
    );

    QPushButton* generateButton = new QPushButton("Generate matrix");
    generateButton->setProperty("primary", true);
    generateButton->setCursor(Qt::PointingHandCursor);

    QPushButton* exportButton =
        new QPushButton("Export CSV / PHYLIP");

    QPushButton* reportButton =
        new QPushButton("HTML report");

    QPushButton* heatmapButton =
        new QPushButton("Open heatmap  →");

    heatmapButton->setProperty("primary", true);

    exportButton->setCursor(Qt::PointingHandCursor);
    reportButton->setCursor(Qt::PointingHandCursor);
    heatmapButton->setCursor(Qt::PointingHandCursor);

    controlRow->addWidget(methodLabel);
    controlRow->addWidget(matrixAlignmentMethodBox, 1);
    controlRow->addStretch();
    controlRow->addWidget(generateButton);
    controlRow->addWidget(exportButton);
    controlRow->addWidget(reportButton);

    controlLayout->addWidget(controlTitle);
    controlLayout->addWidget(controlDescription);
    controlLayout->addLayout(controlRow);

    QFrame* resultCard = new QFrame;
    resultCard->setObjectName("analysisCard");

    QVBoxLayout* resultLayout = new QVBoxLayout(resultCard);
    resultLayout->setContentsMargins(18, 12, 18, 14);
    resultLayout->setSpacing(7);

    QHBoxLayout* resultHeader = new QHBoxLayout;

    QVBoxLayout* resultTitleLayout = new QVBoxLayout;
    resultTitleLayout->setSpacing(2);

    QLabel* resultTitle = new QLabel("Pairwise distance matrix");
    resultTitle->setObjectName("panelTitle");

    QLabel* resultDescription = new QLabel(
        "Rows show full sequence names; columns use S1, S2 and so on in the same order for a clear, compact comparison."
    );
    resultDescription->setObjectName("panelDescription");

    resultTitleLayout->addWidget(resultTitle);
    resultTitleLayout->addWidget(resultDescription);

    matrixStatusLabel =
        new QLabel("No distance matrix generated");

    matrixStatusLabel->setObjectName("resultStatusBadge");
    matrixStatusLabel->setAlignment(Qt::AlignCenter);

    resultHeader->addLayout(resultTitleLayout);
    resultHeader->addStretch();
    resultHeader->addWidget(heatmapButton, 0, Qt::AlignTop);
    resultHeader->addWidget(matrixStatusLabel, 0, Qt::AlignTop);

    distanceMatrixTable = new QTableWidget;

    distanceMatrixTable->setEditTriggers(
        QAbstractItemView::NoEditTriggers
    );

    distanceMatrixTable->setAlternatingRowColors(true);
    distanceMatrixTable->setSelectionMode(QAbstractItemView::SingleSelection);
    distanceMatrixTable->setSelectionBehavior(QAbstractItemView::SelectItems);

    distanceMatrixTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    distanceMatrixTable->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    distanceMatrixTable->verticalHeader()->setDefaultSectionSize(29);
    distanceMatrixTable->setStyleSheet(
        "QHeaderView { background-color: #0F172A; }"
        "QHeaderView::section {"
        " background-color: #0F172A; color: #9FB2CE;"
        " border: 1px solid #334155; padding: 6px; font-weight: 600;"
        "}"
        "QTableCornerButton::section {"
        " background-color: #0F172A; border: 1px solid #334155;"
        "}"
    );

    resultLayout->addLayout(resultHeader);
    resultLayout->addWidget(distanceMatrixTable, 1);

    layout->addWidget(controlCard);
    layout->addWidget(resultCard, 1);

    connect(
        generateButton,
        &QPushButton::clicked,
        this,
        &MainWindow::generateDistanceMatrix
    );

    connect(
        exportButton,
        &QPushButton::clicked,
        this,
        &MainWindow::exportMatrixResults
    );

    connect(
        reportButton,
        &QPushButton::clicked,
        this,
        &MainWindow::generatePhylogeneticHtmlReport
    );

    connect(
        heatmapButton,
        &QPushButton::clicked,
        this,
        &MainWindow::openPhylogeneticHeatmapPage
    );

    return page;
}

QWidget* MainWindow::createPhylogeneticHeatmapPage()
{
    QWidget* page = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(page);
    layout->setContentsMargins(28, 16, 28, 18);
    layout->setSpacing(10);

    QFrame* heatmapCard = new QFrame;
    heatmapCard->setObjectName("analysisCard");

    QVBoxLayout* cardLayout = new QVBoxLayout(heatmapCard);
    cardLayout->setContentsMargins(18, 14, 18, 16);
    cardLayout->setSpacing(9);

    QHBoxLayout* headerLayout = new QHBoxLayout;

    QVBoxLayout* titleLayout = new QVBoxLayout;
    titleLayout->setSpacing(2);

    QLabel* title = new QLabel("Pairwise distance heatmap");
    title->setObjectName("panelTitle");

    QLabel* description = new QLabel(
        "Every coloured cell shows the calculated distance. Column S1 corresponds to the first row, S2 to the second, and so on."
    );
    description->setObjectName("panelDescription");
    description->setWordWrap(true);

    titleLayout->addWidget(title);
    titleLayout->addWidget(description);

    QPushButton* exportButton = new QPushButton("Export heatmap PNG");

    exportButton->setProperty("primary", true);

    exportButton->setCursor(Qt::PointingHandCursor);

    headerLayout->addLayout(titleLayout, 1);
    headerLayout->addWidget(exportButton, 0, Qt::AlignTop);

    QLabel* legend = new QLabel(
        "GREEN  similar        YELLOW  moderate        RED  distant"
    );
    legend->setObjectName("resultStatusBadge");
    legend->setAlignment(Qt::AlignCenter);

    distanceHeatmapTable = new QTableWidget;
    distanceHeatmapTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    distanceHeatmapTable->setSelectionMode(QAbstractItemView::NoSelection);
    distanceHeatmapTable->setFocusPolicy(Qt::NoFocus);
    distanceHeatmapTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    distanceHeatmapTable->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    distanceHeatmapTable->verticalHeader()->setDefaultSectionSize(38);
    distanceHeatmapTable->setStyleSheet(
        "QHeaderView { background-color: #0F172A; }"
        "QHeaderView::section {"
        " background-color: #0F172A; color: #9FB2CE;"
        " border: 1px solid #334155; padding: 6px; font-weight: 600;"
        "}"
        "QTableCornerButton::section {"
        " background-color: #0F172A; border: 1px solid #334155;"
        "}"
    );

    cardLayout->addLayout(headerLayout);
    cardLayout->addWidget(legend, 0, Qt::AlignRight);
    cardLayout->addWidget(distanceHeatmapTable, 1);

    layout->addWidget(heatmapCard, 1);

    connect(
        exportButton,
        &QPushButton::clicked,
        this,
        [this]()
        {
            exportWidgetImage(
                distanceHeatmapTable,
                "distance_heatmap.png",
                "Export Distance Heatmap"
            );
        }
    );

    return page;
}

QWidget* MainWindow::createPhylogeneticTreePage()
{
    QWidget* page = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(page);

    layout->setContentsMargins(28, 16, 28, 18);
    layout->setSpacing(10);

    QFrame* controlCard = new QFrame;
    controlCard->setObjectName("analysisCard");

    QVBoxLayout* controlLayout = new QVBoxLayout(controlCard);
    controlLayout->setContentsMargins(18, 12, 18, 12);
    controlLayout->setSpacing(7);

    QLabel* controlTitle = new QLabel("Tree controls");
    controlTitle->setObjectName("panelTitle");

    QLabel* controlDescription = new QLabel(
        "Select a distance strategy, construct the UPGMA tree and export reproducible results."
    );
    controlDescription->setObjectName("panelDescription");

    QLabel* methodLabel = new QLabel("Distance method");
    methodLabel->setObjectName("fieldLabel");

    treeAlignmentMethodBox = new QComboBox;

    treeAlignmentMethodBox->addItem(
        "Needleman-Wunsch Global Distance"
    );

    treeAlignmentMethodBox->addItem(
        "Hamming Distance"
    );

    treeAlignmentMethodBox->setMinimumWidth(270);

    QHBoxLayout* controlRow = new QHBoxLayout;
    controlRow->setSpacing(10);

    QPushButton* generateButton =
        new QPushButton("Generate tree");

    QPushButton* exportButton =
        new QPushButton("Export Newick / PNG");

    QPushButton* reportButton =
        new QPushButton("HTML report");

    generateButton->setProperty("primary", true);

    generateButton->setCursor(Qt::PointingHandCursor);
    exportButton->setCursor(Qt::PointingHandCursor);
    reportButton->setCursor(Qt::PointingHandCursor);

    generateButton->setToolTip(
        "Calculate the selected distance matrix and construct a UPGMA tree."
    );
    exportButton->setToolTip(
        "Export the tree in Newick format and as a PNG image."
    );
    reportButton->setToolTip(
        "Generate the complete phylogenetic HTML report."
    );

    controlRow->addWidget(methodLabel);
    controlRow->addWidget(treeAlignmentMethodBox, 1);
    controlRow->addStretch();
    controlRow->addWidget(generateButton);
    controlRow->addWidget(exportButton);
    controlRow->addWidget(reportButton);

    controlLayout->addWidget(controlTitle);
    controlLayout->addWidget(controlDescription);
    controlLayout->addLayout(controlRow);

    QFrame* resultCard = new QFrame;
    resultCard->setObjectName("analysisCard");

    QVBoxLayout* resultLayout = new QVBoxLayout(resultCard);
    resultLayout->setContentsMargins(18, 12, 18, 14);
    resultLayout->setSpacing(7);

    QHBoxLayout* resultHeader = new QHBoxLayout;

    QVBoxLayout* resultTitleLayout = new QVBoxLayout;
    resultTitleLayout->setSpacing(2);

    QLabel* resultTitle = new QLabel("UPGMA tree visualization");
    resultTitle->setObjectName("panelTitle");

    QLabel* resultDescription = new QLabel(
        "Branch structure reflects hierarchical clustering from the selected pairwise distances."
    );
    resultDescription->setObjectName("panelDescription");

    resultTitleLayout->addWidget(resultTitle);
    resultTitleLayout->addWidget(resultDescription);

    treeStatusLabel =
        new QLabel("No phylogenetic tree generated");

    treeStatusLabel->setObjectName("resultStatusBadge");
    treeStatusLabel->setAlignment(Qt::AlignCenter);

    resultHeader->addLayout(resultTitleLayout);
    resultHeader->addStretch();
    resultHeader->addWidget(treeStatusLabel, 0, Qt::AlignTop);

    treeGraphic =
        new PhylogeneticTreeWidget;

    treeGraphic->setMinimumSize(660, 250);
    treeGraphic->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QScrollArea* treeScrollArea = new QScrollArea;
    treeScrollArea->setWidget(treeGraphic);
    treeScrollArea->setWidgetResizable(true);
    treeScrollArea->setMinimumHeight(250);
    treeScrollArea->setFrameShape(QFrame::NoFrame);
    treeScrollArea->setStyleSheet(
        "QScrollArea {"
        "background-color: #0F172A;"
        "border: 1px solid #334155;"
        "border-radius: 8px;"
        "}"
        "QScrollArea > QWidget > QWidget { background-color: #0F172A; }"
    );

    QFrame* newickPanel = new QFrame;
    newickPanel->setMinimumWidth(260);
    newickPanel->setMaximumWidth(360);
    newickPanel->setStyleSheet(
        "QFrame {"
        " background-color: #0F172A;"
        " border: 1px solid #334155;"
        " border-radius: 8px;"
        "}"
        "QLabel, QPlainTextEdit { border: none; }"
    );

    QVBoxLayout* newickLayout = new QVBoxLayout(newickPanel);
    newickLayout->setContentsMargins(14, 12, 14, 14);
    newickLayout->setSpacing(7);

    QLabel* newickLabel = new QLabel("Newick representation");
    newickLabel->setObjectName("panelTitle");

    QLabel* newickHint = new QLabel(
        "Machine-readable tree format for downstream tools and reproducible exports."
    );
    newickHint->setObjectName("panelHint");
    newickHint->setWordWrap(true);

    treeOutput = new QPlainTextEdit;

    treeOutput->setReadOnly(true);
    treeOutput->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    treeOutput->setPlaceholderText(
        "Newick representation will appear here."
    );

    newickLayout->addWidget(newickLabel);
    newickLayout->addWidget(newickHint);
    newickLayout->addWidget(treeOutput, 1);

    QHBoxLayout* visualizationLayout = new QHBoxLayout;
    visualizationLayout->setSpacing(10);
    visualizationLayout->addWidget(treeScrollArea, 3);
    visualizationLayout->addWidget(newickPanel, 1);

    resultLayout->addLayout(resultHeader);
    resultLayout->addLayout(visualizationLayout, 1);

    layout->addWidget(controlCard);
    layout->addWidget(resultCard, 1);

    connect(
        generateButton,
        &QPushButton::clicked,
        this,
        &MainWindow::generatePhylogeneticTree
    );

    connect(
        exportButton,
        &QPushButton::clicked,
        this,
        &MainWindow::exportTreeResults
    );

    connect(
        reportButton,
        &QPushButton::clicked,
        this,
        &MainWindow::generatePhylogeneticHtmlReport
    );

    return page;
}

QWidget* MainWindow::createGeneExpressionPage()
{
    QWidget* page = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(page);

    layout->setContentsMargins(28, 16, 28, 18);
    layout->setSpacing(10);

    QHBoxLayout* workspaceLayout = new QHBoxLayout;
    workspaceLayout->setSpacing(12);

    QFrame* datasetCard = new QFrame;
    datasetCard->setObjectName("analysisCard");

    QVBoxLayout* datasetLayout = new QVBoxLayout(datasetCard);
    datasetLayout->setContentsMargins(18, 14, 18, 16);
    datasetLayout->setSpacing(9);

    QHBoxLayout* datasetHeader = new QHBoxLayout;
    QVBoxLayout* datasetTitleLayout = new QVBoxLayout;
    datasetTitleLayout->setSpacing(2);

    QLabel* datasetTitle = new QLabel("Expression dataset");
    datasetTitle->setObjectName("panelTitle");

    QLabel* datasetDescription = new QLabel(
        "Import a CSV or TSV matrix with genes in rows and biological samples in columns."
    );
    datasetDescription->setObjectName("panelDescription");
    datasetDescription->setWordWrap(true);

    datasetTitleLayout->addWidget(datasetTitle);
    datasetTitleLayout->addWidget(datasetDescription);

    QPushButton* importButton =
        new QPushButton("Import CSV / TSV");
    importButton->setProperty("primary", true);
    importButton->setCursor(Qt::PointingHandCursor);

    datasetHeader->addLayout(datasetTitleLayout, 1);
    datasetHeader->addWidget(importButton, 0, Qt::AlignTop);

    expressionFileLabel =
        new QLabel("No expression file selected");

    expressionFileLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    expressionFileLabel->setWordWrap(true);
    expressionFileLabel->setObjectName("resultStatusBadge");

    expressionSummaryLabel =
        new QLabel(
            "Import a dataset to view its summary."
        );

    expressionSummaryLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    expressionSummaryLabel->setObjectName("panelHint");

    expressionPreviewTable =
        new QTableWidget;

    styleDashboardTable(expressionPreviewTable);
    expressionPreviewTable->horizontalHeader()->setSectionResizeMode(
        QHeaderView::Stretch
    );

    datasetLayout->addLayout(datasetHeader);
    datasetLayout->addWidget(expressionFileLabel);
    datasetLayout->addWidget(expressionSummaryLabel);
    datasetLayout->addWidget(expressionPreviewTable, 1);

    QFrame* actionCard = new QFrame;
    actionCard->setObjectName("analysisCard");
    actionCard->setMinimumWidth(300);
    actionCard->setMaximumWidth(390);

    QVBoxLayout* actionLayout = new QVBoxLayout(actionCard);
    actionLayout->setContentsMargins(18, 14, 18, 16);
    actionLayout->setSpacing(10);

    QLabel* actionTitle = new QLabel("Analysis workflow");
    actionTitle->setObjectName("panelTitle");

    QLabel* actionDescription = new QLabel(
        "Validate the imported matrix, then configure groups, normalization and statistical testing."
    );
    actionDescription->setObjectName("panelDescription");
    actionDescription->setWordWrap(true);

    configureExpressionButton =
        new QPushButton("Configure differential expression  →");
    configureExpressionButton->setObjectName("analysisOptionButton");
    configureExpressionButton->setEnabled(false);

    expressionQualityButton =
        new QPushButton("Expression quality control  →");
    expressionQualityButton->setObjectName("analysisOptionButton");
    expressionQualityButton->setEnabled(false);

    configureExpressionButton->setCursor(Qt::PointingHandCursor);
    expressionQualityButton->setCursor(Qt::PointingHandCursor);

    QLabel* privacyHint = new QLabel(
        "Analysis runs locally. Expression data is not uploaded."
    );
    privacyHint->setObjectName("panelHint");
    privacyHint->setWordWrap(true);

    actionLayout->addWidget(actionTitle);
    actionLayout->addWidget(actionDescription);
    actionLayout->addSpacing(5);
    actionLayout->addWidget(expressionQualityButton);
    actionLayout->addWidget(configureExpressionButton);
    actionLayout->addStretch();
    actionLayout->addWidget(privacyHint);

    workspaceLayout->addWidget(datasetCard, 3);
    workspaceLayout->addWidget(actionCard, 2);
    layout->addLayout(workspaceLayout, 1);

    connect(
        importButton,
        &QPushButton::clicked,
        this,
        &MainWindow::importExpressionFile
    );

    connect(
        configureExpressionButton,
        &QPushButton::clicked,
        this,
        &MainWindow::openExpressionConfigurationPage
    );

    connect(
        expressionQualityButton,
        &QPushButton::clicked,
        this,
        &MainWindow::openExpressionQualityPage
    );

    return page;
}

QWidget* MainWindow::createExpressionConfigurationPage()
{
    QWidget* page = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(page);

    layout->setContentsMargins(28, 16, 28, 18);
    layout->setSpacing(10);

    QFrame* configurationCard = new QFrame;
    configurationCard->setObjectName("analysisCard");

    QVBoxLayout* cardLayout = new QVBoxLayout(configurationCard);
    cardLayout->setContentsMargins(18, 14, 18, 16);
    cardLayout->setSpacing(9);

    QHBoxLayout* headerLayout = new QHBoxLayout;
    QVBoxLayout* titleLayout = new QVBoxLayout;
    titleLayout->setSpacing(2);

    QLabel* title = new QLabel("Sample groups and normalization");
    title->setObjectName("panelTitle");

    QLabel* description = new QLabel(
        "Assign Control or Treatment groups, select normalization and run Welch's independent t-test."
    );
    description->setObjectName("panelDescription");
    description->setWordWrap(true);

    titleLayout->addWidget(title);
    titleLayout->addWidget(description);

    sampleGroupingTable = new QTableWidget;
    sampleGroupingTable->setColumnCount(2);
    sampleGroupingTable->setHorizontalHeaderLabels(
        {"Sample", "Experimental Group"}
    );
    styleDashboardTable(sampleGroupingTable);
    sampleGroupingTable->horizontalHeader()->setSectionResizeMode(
        QHeaderView::Stretch
    );

    QLabel* normalizationLabel = new QLabel("Normalization strategy");
    normalizationLabel->setObjectName("fieldLabel");

    normalizationMethodBox = new QComboBox;
    normalizationMethodBox->addItems(
        {
            "Raw values (no transformation)",
            "Log2 transformation: log2(x + 1)",
            "Z-score normalization per gene"
        }
    );

    groupingStatusLabel = new QLabel(
        "Import a dataset before configuring the analysis."
    );
    groupingStatusLabel->setAlignment(Qt::AlignCenter);
    groupingStatusLabel->setWordWrap(true);
    groupingStatusLabel->setObjectName("resultStatusBadge");

    headerLayout->addLayout(titleLayout, 1);
    headerLayout->addWidget(groupingStatusLabel, 0, Qt::AlignTop);

    QPushButton* runButton = new QPushButton(
        "Run differential expression"
    );
    runButton->setProperty("primary", true);
    runButton->setCursor(Qt::PointingHandCursor);

    QHBoxLayout* footerLayout = new QHBoxLayout;
    footerLayout->setSpacing(10);
    footerLayout->addWidget(normalizationLabel);
    footerLayout->addWidget(normalizationMethodBox, 1);
    footerLayout->addStretch();
    footerLayout->addWidget(runButton);

    cardLayout->addLayout(headerLayout);
    cardLayout->addWidget(sampleGroupingTable, 1);
    cardLayout->addLayout(footerLayout);

    layout->addWidget(configurationCard, 1);

    connect(
        runButton,
        &QPushButton::clicked,
        this,
        &MainWindow::runDifferentialExpressionAnalysis
    );

    return page;
}

QWidget* MainWindow::createExpressionResultsPage()
{
    QWidget* page = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(page);

    layout->setContentsMargins(28, 16, 28, 18);
    layout->setSpacing(10);

    QFrame* filterCard = new QFrame;
    filterCard->setObjectName("analysisCard");

    QVBoxLayout* filterCardLayout = new QVBoxLayout(filterCard);
    filterCardLayout->setContentsMargins(18, 12, 18, 12);
    filterCardLayout->setSpacing(7);

    QLabel* filterTitle = new QLabel("Result filters");
    filterTitle->setObjectName("panelTitle");

    QLabel* filterDescription = new QLabel(
        "Search, classify and prioritize genes using statistical and fold-change thresholds."
    );
    filterDescription->setObjectName("panelDescription");

    expressionResultsSummaryLabel = new QLabel(
        "Run an analysis to generate results."
    );
    expressionResultsSummaryLabel->setAlignment(
        Qt::AlignLeft | Qt::AlignVCenter
    );
    expressionResultsSummaryLabel->setWordWrap(true);
    expressionResultsSummaryLabel->setObjectName("resultStatusBadge");

    QHBoxLayout* filterLayout = new QHBoxLayout;
    filterLayout->setSpacing(10);

    QLabel* searchLabel = new QLabel("Search gene:");
    searchLabel->setObjectName("fieldLabel");

    geneSearchBox = new QLineEdit;
    geneSearchBox->setPlaceholderText("Example: TP53");
    geneSearchBox->setClearButtonEnabled(true);

    QLabel* regulationLabel = new QLabel("Regulation:");
    regulationLabel->setObjectName("fieldLabel");

    regulationFilterBox = new QComboBox;
    regulationFilterBox->addItems(
        {
            "All genes",
            "Upregulated",
            "Downregulated",
            "Not significant"
        }
    );

    QLabel* maximumLabel = new QLabel("Display:");
    maximumLabel->setObjectName("fieldLabel");

    maximumResultsBox = new QComboBox;
    maximumResultsBox->addItem("All results", 0);
    maximumResultsBox->addItem("Top 10", 10);
    maximumResultsBox->addItem("Top 20", 20);
    maximumResultsBox->addItem("Top 50", 50);

    QLabel* pValueLabel = new QLabel("Adjusted p-value <=");
    pValueLabel->setObjectName("fieldLabel");

    adjustedPThresholdBox = new QDoubleSpinBox;
    adjustedPThresholdBox->setRange(0.000001, 1.0);
    adjustedPThresholdBox->setDecimals(6);
    adjustedPThresholdBox->setSingleStep(0.01);
    adjustedPThresholdBox->setValue(0.05);

    QLabel* foldChangeLabel = new QLabel("Minimum |log2 FC|:");
    foldChangeLabel->setObjectName("fieldLabel");

    foldChangeThresholdBox = new QDoubleSpinBox;
    foldChangeThresholdBox->setRange(0.0, 20.0);
    foldChangeThresholdBox->setDecimals(2);
    foldChangeThresholdBox->setSingleStep(0.25);
    foldChangeThresholdBox->setValue(1.0);

    QPushButton* resetFiltersButton = new QPushButton(
        "Reset Filters"
    );
    resetFiltersButton->setObjectName("compactActionButton");

    expressionFilterSummaryLabel = new QLabel(
        "Run an analysis to enable interactive filtering."
    );
    expressionFilterSummaryLabel->setAlignment(Qt::AlignCenter);
    expressionFilterSummaryLabel->setWordWrap(true);
    expressionFilterSummaryLabel->setObjectName("panelHint");

    QVBoxLayout* searchField = new QVBoxLayout;
    searchField->setSpacing(4);
    searchField->addWidget(searchLabel);
    searchField->addWidget(geneSearchBox);

    QVBoxLayout* regulationField = new QVBoxLayout;
    regulationField->setSpacing(4);
    regulationField->addWidget(regulationLabel);
    regulationField->addWidget(regulationFilterBox);

    QVBoxLayout* pValueField = new QVBoxLayout;
    pValueField->setSpacing(4);
    pValueField->addWidget(pValueLabel);
    pValueField->addWidget(adjustedPThresholdBox);

    QVBoxLayout* foldChangeField = new QVBoxLayout;
    foldChangeField->setSpacing(4);
    foldChangeField->addWidget(foldChangeLabel);
    foldChangeField->addWidget(foldChangeThresholdBox);

    QVBoxLayout* displayField = new QVBoxLayout;
    displayField->setSpacing(4);
    displayField->addWidget(maximumLabel);
    displayField->addWidget(maximumResultsBox);

    filterLayout->addLayout(searchField, 2);
    filterLayout->addLayout(regulationField, 1);
    filterLayout->addLayout(pValueField, 1);
    filterLayout->addLayout(foldChangeField, 1);
    filterLayout->addLayout(displayField, 1);
    filterLayout->addWidget(resetFiltersButton, 0, Qt::AlignBottom);

    filterCardLayout->addWidget(filterTitle);
    filterCardLayout->addWidget(filterDescription);
    filterCardLayout->addLayout(filterLayout);
    filterCardLayout->addWidget(expressionFilterSummaryLabel);

    QFrame* resultCard = new QFrame;
    resultCard->setObjectName("analysisCard");

    QVBoxLayout* resultCardLayout = new QVBoxLayout(resultCard);
    resultCardLayout->setContentsMargins(18, 12, 18, 14);
    resultCardLayout->setSpacing(7);

    QHBoxLayout* resultHeader = new QHBoxLayout;

    QVBoxLayout* resultTitleLayout = new QVBoxLayout;
    resultTitleLayout->setSpacing(2);

    QLabel* resultTitle = new QLabel("Differential expression table");
    resultTitle->setObjectName("panelTitle");

    QLabel* resultDescription = new QLabel(
        "Click a column heading to sort genes; adjusted p-values use Benjamini-Hochberg correction."
    );
    resultDescription->setObjectName("panelDescription");

    resultTitleLayout->addWidget(resultTitle);
    resultTitleLayout->addWidget(resultDescription);

    expressionResultsTable = new QTableWidget;
    expressionResultsTable->setColumnCount(7);
    expressionResultsTable->setHorizontalHeaderLabels(
        {
            "Gene",
            "Control Mean",
            "Treatment Mean",
            "Log2 Fold Change",
            "P-value",
            "Adjusted P-value",
            "Regulation"
        }
    );
    styleDashboardTable(expressionResultsTable);
    expressionResultsTable->horizontalHeader()->setSectionResizeMode(
        0, QHeaderView::ResizeToContents
    );
    for (int column = 1; column < 7; ++column)
    {
        expressionResultsTable->horizontalHeader()->setSectionResizeMode(
            column, QHeaderView::Stretch
        );
    }

    QPushButton* volcanoButton = new QPushButton(
        "Volcano"
    );
    volcanoButton->setToolTip(
        "Explore fold change and statistical significance interactively."
    );

    QPushButton* heatmapButton = new QPushButton(
        "Heatmap"
    );
    heatmapButton->setToolTip(
        "Compare relative expression patterns across samples."
    );

    QPushButton* pcaButton = new QPushButton(
        "PCA"
    );
    pcaButton->setToolTip(
        "Inspect sample clustering using principal component analysis."
    );

    QPushButton* enrichmentButton = new QPushButton(
        "Enrichment"
    );
    enrichmentButton->setToolTip(
        "Analyze enriched biological functions and pathways."
    );

    QPushButton* exportTablesButton = new QPushButton(
        "Export tables"
    );
    exportTablesButton->setToolTip(
        "Export differential-expression tables and analysis summary."
    );

    QPushButton* reportButton = new QPushButton(
        "HTML report"
    );
    reportButton->setToolTip(
        "Generate the complete interactive analysis report."
    );

    const std::vector<QPushButton*> explorationButtons = {
        volcanoButton,
        heatmapButton,
        pcaButton,
        enrichmentButton
    };

    for (QPushButton* button : explorationButtons)
    {
        button->setObjectName("compactActionButton");
        button->setCursor(Qt::PointingHandCursor);
    }

    exportTablesButton->setObjectName("compactPrimaryButton");
    reportButton->setObjectName("compactPrimaryButton");
    exportTablesButton->setCursor(Qt::PointingHandCursor);
    reportButton->setCursor(Qt::PointingHandCursor);

    QLabel* exploreLabel = new QLabel("Explore");
    exploreLabel->setObjectName("fieldLabel");

    QHBoxLayout* explorationLayout = new QHBoxLayout;
    explorationLayout->setSpacing(7);
    explorationLayout->addWidget(exploreLabel);
    explorationLayout->addWidget(volcanoButton);
    explorationLayout->addWidget(heatmapButton);
    explorationLayout->addWidget(pcaButton);
    explorationLayout->addWidget(enrichmentButton);
    explorationLayout->addStretch();

    resultHeader->addLayout(resultTitleLayout, 1);
    resultHeader->addWidget(exportTablesButton, 0, Qt::AlignTop);
    resultHeader->addWidget(reportButton, 0, Qt::AlignTop);

    resultCardLayout->addLayout(resultHeader);
    resultCardLayout->addWidget(
        expressionResultsSummaryLabel
    );
    resultCardLayout->addWidget(expressionResultsTable, 1);
    resultCardLayout->addLayout(explorationLayout);

    layout->addWidget(filterCard);
    layout->addWidget(resultCard, 1);

    connect(
        geneSearchBox,
        &QLineEdit::textChanged,
        this,
        [this]() { applyExpressionFilters(); }
    );

    connect(
        regulationFilterBox,
        &QComboBox::currentIndexChanged,
        this,
        [this]() { applyExpressionFilters(); }
    );

    connect(
        maximumResultsBox,
        &QComboBox::currentIndexChanged,
        this,
        [this]() { applyExpressionFilters(); }
    );

    connect(
        adjustedPThresholdBox,
        &QDoubleSpinBox::valueChanged,
        this,
        [this]() { applyExpressionFilters(); }
    );

    connect(
        foldChangeThresholdBox,
        &QDoubleSpinBox::valueChanged,
        this,
        [this]() { applyExpressionFilters(); }
    );

    connect(
        resetFiltersButton,
        &QPushButton::clicked,
        this,
        &MainWindow::resetExpressionFilters
    );

    connect(
        volcanoButton,
        &QPushButton::clicked,
        this,
        &MainWindow::openExpressionVolcanoPage
    );

    connect(
        heatmapButton,
        &QPushButton::clicked,
        this,
        &MainWindow::openExpressionHeatmapPage
    );

    connect(
        pcaButton,
        &QPushButton::clicked,
        this,
        &MainWindow::openExpressionPCAPage
    );

    connect(
        enrichmentButton,
        &QPushButton::clicked,
        this,
        &MainWindow::openExpressionEnrichmentPage
    );

    connect(
        exportTablesButton,
        &QPushButton::clicked,
        this,
        &MainWindow::exportExpressionTables
    );

    connect(
        reportButton,
        &QPushButton::clicked,
        this,
        &MainWindow::generateExpressionHtmlReport
    );

    return page;
}

QWidget* MainWindow::createExpressionVolcanoPage()
{
    QWidget* page = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(page);

    layout->setContentsMargins(28, 16, 28, 18);

    QFrame* plotCard = new QFrame;
    plotCard->setObjectName("analysisCard");

    QVBoxLayout* cardLayout = new QVBoxLayout(plotCard);
    cardLayout->setContentsMargins(18, 14, 18, 16);
    cardLayout->setSpacing(9);

    QHBoxLayout* headerLayout = new QHBoxLayout;
    QVBoxLayout* titleLayout = new QVBoxLayout;
    titleLayout->setSpacing(2);

    QLabel* title = new QLabel("Interactive volcano plot");
    title->setObjectName("panelTitle");

    QLabel* description = new QLabel(
        "Green points are upregulated, red points are downregulated, "
        "and grey points are not significant. Hover for gene details and drag to zoom."
    );
    description->setObjectName("panelDescription");
    description->setWordWrap(true);

    titleLayout->addWidget(title);
    titleLayout->addWidget(description);

    volcanoPlotWidget = new VolcanoPlotWidget;

    QPushButton* resetZoomButton = new QPushButton(
        "Reset zoom"
    );

    QPushButton* exportButton = new QPushButton(
        "Export PNG"
    );

    resetZoomButton->setObjectName("compactActionButton");
    exportButton->setProperty("primary", true);
    resetZoomButton->setCursor(Qt::PointingHandCursor);
    exportButton->setCursor(Qt::PointingHandCursor);

    headerLayout->addLayout(titleLayout, 1);
    headerLayout->addWidget(resetZoomButton, 0, Qt::AlignTop);
    headerLayout->addWidget(exportButton, 0, Qt::AlignTop);

    cardLayout->addLayout(headerLayout);
    cardLayout->addWidget(volcanoPlotWidget, 1);
    layout->addWidget(plotCard, 1);

    connect(
        resetZoomButton,
        &QPushButton::clicked,
        this,
        [this]()
        {
            volcanoPlotWidget->chart()->zoomReset();
        }
    );

    connect(
        exportButton,
        &QPushButton::clicked,
        this,
        [this]()
        {
            exportWidgetImage(
                volcanoPlotWidget,
                "volcano_plot.png",
                "Export Volcano Plot"
            );
        }
    );

    return page;
}

QWidget* MainWindow::createExpressionHeatmapPage()
{
    QWidget* page = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(page);

    layout->setContentsMargins(28, 16, 28, 18);

    QFrame* heatmapCard = new QFrame;
    heatmapCard->setObjectName("analysisCard");

    QVBoxLayout* cardLayout = new QVBoxLayout(heatmapCard);
    cardLayout->setContentsMargins(18, 14, 18, 16);
    cardLayout->setSpacing(8);

    QHBoxLayout* headerLayout = new QHBoxLayout;
    QVBoxLayout* titleLayout = new QVBoxLayout;
    titleLayout->setSpacing(2);

    QLabel* title = new QLabel("Gene-expression heatmap");
    title->setObjectName("panelTitle");

    QLabel* description = new QLabel(
        "The most statistically important genes are ordered by adjusted "
        "p-value; row-wise colours compare relative expression across samples."
    );
    description->setObjectName("panelDescription");
    description->setWordWrap(true);

    titleLayout->addWidget(title);
    titleLayout->addWidget(description);

    expressionHeatmapSummaryLabel = new QLabel(
        "Run an analysis to generate a heatmap."
    );
    expressionHeatmapSummaryLabel->setAlignment(
        Qt::AlignLeft | Qt::AlignVCenter
    );
    expressionHeatmapSummaryLabel->setWordWrap(true);
    expressionHeatmapSummaryLabel->setObjectName("resultStatusBadge");

    QHBoxLayout* legendLayout = new QHBoxLayout;
    QLabel* lowLabel = new QLabel("  Lower expression  ");
    lowLabel->setAlignment(Qt::AlignCenter);
    lowLabel->setStyleSheet(
        "background-color: #2166AC; color: white; padding: 6px;"
    );

    QLabel* averageLabel = new QLabel("  Gene average  ");
    averageLabel->setAlignment(Qt::AlignCenter);
    averageLabel->setStyleSheet(
        "background-color: #FAFAFA; color: #203040; "
        "border: 1px solid #D7E0E8; padding: 6px;"
    );

    QLabel* highLabel = new QLabel("  Higher expression  ");
    highLabel->setAlignment(Qt::AlignCenter);
    highLabel->setStyleSheet(
        "background-color: #B2182B; color: white; padding: 6px;"
    );

    legendLayout->addWidget(lowLabel);
    legendLayout->addWidget(averageLabel);
    legendLayout->addWidget(highLabel);
    legendLayout->addStretch();

    expressionHeatmapWidget = new ExpressionHeatmapWidget;
    expressionHeatmapWidget->setStyleSheet(
        "QHeaderView { background-color: #0F172A; }"
        "QHeaderView::section {"
        " background-color: #0F172A; color: #9FB2CE;"
        " border: 1px solid #334155; padding: 6px; font-weight: 600;"
        "}"
        "QTableCornerButton::section {"
        " background-color: #0F172A; border: 1px solid #334155;"
        "}"
        "QTableView { background-color: #1E293B; }"
    );
    fitEmbeddedHeatmapTable(expressionHeatmapWidget);

    QPushButton* exportButton = new QPushButton(
        "Export PNG"
    );
    exportButton->setProperty("primary", true);
    exportButton->setCursor(Qt::PointingHandCursor);

    headerLayout->addLayout(titleLayout, 1);
    headerLayout->addWidget(exportButton, 0, Qt::AlignTop);

    cardLayout->addLayout(headerLayout);
    cardLayout->addWidget(expressionHeatmapSummaryLabel);
    cardLayout->addLayout(legendLayout);
    cardLayout->addWidget(expressionHeatmapWidget, 1);
    layout->addWidget(heatmapCard, 1);

    connect(
        exportButton,
        &QPushButton::clicked,
        this,
        [this]()
        {
            exportWidgetImage(
                expressionHeatmapWidget,
                "expression_heatmap.png",
                "Export Expression Heatmap"
            );
        }
    );

    return page;
}

QWidget* MainWindow::createExpressionPCAPage()
{
    QWidget* page = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(page);

    layout->setContentsMargins(28, 16, 28, 18);

    QFrame* pcaCard = new QFrame;
    pcaCard->setObjectName("analysisCard");

    QVBoxLayout* cardLayout = new QVBoxLayout(pcaCard);
    cardLayout->setContentsMargins(18, 14, 18, 16);
    cardLayout->setSpacing(9);

    QHBoxLayout* headerLayout = new QHBoxLayout;
    QVBoxLayout* titleLayout = new QVBoxLayout;
    titleLayout->setSpacing(2);

    QLabel* title = new QLabel("PCA sample clustering");
    title->setObjectName("panelTitle");

    QLabel* description = new QLabel(
        "Each point represents one biological sample. Samples positioned "
        "near each other have similar overall expression profiles. Hover for sample details."
    );
    description->setObjectName("panelDescription");
    description->setWordWrap(true);

    titleLayout->addWidget(title);
    titleLayout->addWidget(description);

    pcaSummaryLabel = new QLabel(
        "Run an analysis to calculate PCA."
    );
    pcaSummaryLabel->setAlignment(Qt::AlignCenter);
    pcaSummaryLabel->setWordWrap(true);
    pcaSummaryLabel->setObjectName("resultStatusBadge");

    pcaPlotWidget = new PCAPlotWidget;

    QPushButton* resetZoomButton = new QPushButton(
        "Reset zoom"
    );

    QPushButton* exportButton = new QPushButton(
        "Export PNG"
    );

    resetZoomButton->setObjectName("compactActionButton");
    exportButton->setProperty("primary", true);
    resetZoomButton->setCursor(Qt::PointingHandCursor);
    exportButton->setCursor(Qt::PointingHandCursor);

    headerLayout->addLayout(titleLayout, 1);
    headerLayout->addWidget(resetZoomButton, 0, Qt::AlignTop);
    headerLayout->addWidget(exportButton, 0, Qt::AlignTop);

    cardLayout->addLayout(headerLayout);
    cardLayout->addWidget(pcaSummaryLabel, 0, Qt::AlignRight);
    cardLayout->addWidget(pcaPlotWidget, 1);
    layout->addWidget(pcaCard, 1);

    connect(
        resetZoomButton,
        &QPushButton::clicked,
        this,
        [this]()
        {
            pcaPlotWidget->chart()->zoomReset();
        }
    );

    connect(
        exportButton,
        &QPushButton::clicked,
        this,
        [this]()
        {
            exportWidgetImage(
                pcaPlotWidget,
                "pca_sample_plot.png",
                "Export PCA Sample Plot"
            );
        }
    );

    return page;
}

QWidget* MainWindow::createPhylogeneticQualityPage()
{
    QWidget* page = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(page);

    layout->setContentsMargins(28, 16, 28, 18);
    layout->setSpacing(10);

    QFrame* qualityCard = new QFrame;
    qualityCard->setObjectName("analysisCard");

    QVBoxLayout* cardLayout = new QVBoxLayout(qualityCard);
    cardLayout->setContentsMargins(18, 14, 18, 16);
    cardLayout->setSpacing(9);

    QHBoxLayout* headerLayout = new QHBoxLayout;

    QVBoxLayout* titleLayout = new QVBoxLayout;
    titleLayout->setSpacing(2);

    QLabel* title = new QLabel("FASTA quality report");
    title->setObjectName("panelTitle");

    QLabel* description = new QLabel(
        "Automatic checks for sequence counts, identifiers, lengths, characters, GC content and distance-method compatibility."
    );
    description->setObjectName("panelDescription");
    description->setWordWrap(true);

    titleLayout->addWidget(title);
    titleLayout->addWidget(description);

    phylogeneticQualitySummaryLabel = new QLabel(
        "Import FASTA data to generate a quality report."
    );
    phylogeneticQualitySummaryLabel->setAlignment(Qt::AlignCenter);
    phylogeneticQualitySummaryLabel->setWordWrap(true);
    phylogeneticQualitySummaryLabel->setObjectName("resultStatusBadge");

    headerLayout->addLayout(titleLayout, 1);
    headerLayout->addWidget(
        phylogeneticQualitySummaryLabel,
        0,
        Qt::AlignTop
    );

    phylogeneticQualityTable = new QTableWidget;
    phylogeneticQualityTable->setColumnCount(4);
    phylogeneticQualityTable->setHorizontalHeaderLabels(
        {"Quality Check", "Status", "Observation", "Recommendation"}
    );
    phylogeneticQualityTable->setEditTriggers(
        QAbstractItemView::NoEditTriggers
    );
    phylogeneticQualityTable->setSelectionBehavior(
        QAbstractItemView::SelectRows
    );
    phylogeneticQualityTable->setSelectionMode(
        QAbstractItemView::SingleSelection
    );
    phylogeneticQualityTable->setAlternatingRowColors(true);
    phylogeneticQualityTable->setWordWrap(true);
    phylogeneticQualityTable->verticalHeader()->setVisible(false);
    phylogeneticQualityTable->horizontalHeader()->setSectionResizeMode(
        0,
        QHeaderView::ResizeToContents
    );
    phylogeneticQualityTable->horizontalHeader()->setSectionResizeMode(
        1,
        QHeaderView::ResizeToContents
    );
    phylogeneticQualityTable->horizontalHeader()->setSectionResizeMode(
        2,
        QHeaderView::Stretch
    );
    phylogeneticQualityTable->horizontalHeader()->setSectionResizeMode(
        3,
        QHeaderView::Stretch
    );
    phylogeneticQualityTable->setStyleSheet(
        "QHeaderView { background-color: #0F172A; }"
        "QHeaderView::section {"
        " background-color: #0F172A; color: #9FB2CE;"
        " border: 1px solid #334155; padding: 8px; font-weight: 600;"
        "}"
        "QTableCornerButton::section {"
        " background-color: #0F172A; border: 1px solid #334155;"
        "}"
    );

    QLabel* tableHint = new QLabel(
        "PASS meets the requirement  •  WARNING needs review  •  FAIL should be corrected before analysis"
    );
    tableHint->setObjectName("panelHint");

    cardLayout->addLayout(headerLayout);
    cardLayout->addWidget(tableHint);
    cardLayout->addWidget(phylogeneticQualityTable, 1);

    layout->addWidget(qualityCard, 1);

    return page;
}

QWidget* MainWindow::createExpressionQualityPage()
{
    QWidget* page = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(page);

    layout->setContentsMargins(28, 16, 28, 18);

    QFrame* qualityCard = new QFrame;
    qualityCard->setObjectName("analysisCard");

    QVBoxLayout* cardLayout = new QVBoxLayout(qualityCard);
    cardLayout->setContentsMargins(18, 14, 18, 16);
    cardLayout->setSpacing(9);

    QHBoxLayout* headerLayout = new QHBoxLayout;
    QVBoxLayout* titleLayout = new QVBoxLayout;
    titleLayout->setSpacing(2);

    QLabel* title = new QLabel("Expression quality report");
    title->setObjectName("panelTitle");

    QLabel* description = new QLabel(
        "Automatic checks for completeness, identifiers, zero values, constant genes, sample totals and replicate readiness."
    );
    description->setObjectName("panelDescription");
    description->setWordWrap(true);

    titleLayout->addWidget(title);
    titleLayout->addWidget(description);

    expressionQualitySummaryLabel = new QLabel(
        "Import expression data to generate a quality report."
    );
    expressionQualitySummaryLabel->setAlignment(Qt::AlignCenter);
    expressionQualitySummaryLabel->setWordWrap(true);
    expressionQualitySummaryLabel->setObjectName("resultStatusBadge");

    headerLayout->addLayout(titleLayout, 1);
    headerLayout->addWidget(
        expressionQualitySummaryLabel,
        0,
        Qt::AlignTop
    );

    expressionQualityTable = new QTableWidget;
    expressionQualityTable->setColumnCount(4);
    expressionQualityTable->setHorizontalHeaderLabels(
        {"Quality Check", "Status", "Observation", "Recommendation"}
    );
    styleDashboardTable(expressionQualityTable);
    expressionQualityTable->setWordWrap(true);
    expressionQualityTable->horizontalHeader()->setSectionResizeMode(
        0, QHeaderView::ResizeToContents
    );
    expressionQualityTable->horizontalHeader()->setSectionResizeMode(
        1, QHeaderView::ResizeToContents
    );
    expressionQualityTable->horizontalHeader()->setSectionResizeMode(
        2, QHeaderView::Stretch
    );
    expressionQualityTable->horizontalHeader()->setSectionResizeMode(
        3, QHeaderView::Stretch
    );

    QLabel* tableHint = new QLabel(
        "PASS meets the requirement  •  WARNING needs review  •  FAIL should be corrected before analysis"
    );
    tableHint->setObjectName("panelHint");

    cardLayout->addLayout(headerLayout);
    cardLayout->addWidget(tableHint);
    cardLayout->addWidget(expressionQualityTable, 1);

    layout->addWidget(qualityCard, 1);

    return page;
}

QWidget* MainWindow::createExpressionEnrichmentPage()
{
    QWidget* page = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(page);

    layout->setContentsMargins(28, 16, 28, 18);
    layout->setSpacing(10);

    QFrame* filterCard = new QFrame;
    filterCard->setObjectName("analysisCard");

    QVBoxLayout* filterCardLayout = new QVBoxLayout(filterCard);
    filterCardLayout->setContentsMargins(18, 12, 18, 12);
    filterCardLayout->setSpacing(7);

    QLabel* filterTitle = new QLabel("Pathway filters");
    filterTitle->setObjectName("panelTitle");

    QLabel* filterDescription = new QLabel(
        "Search and prioritize overrepresented biological functions using adjusted significance thresholds."
    );
    filterDescription->setObjectName("panelDescription");

    enrichmentSummaryLabel = new QLabel(
        "Run differential-expression analysis to identify enriched pathways."
    );
    enrichmentSummaryLabel->setAlignment(
        Qt::AlignLeft | Qt::AlignVCenter
    );
    enrichmentSummaryLabel->setWordWrap(true);
    enrichmentSummaryLabel->setObjectName("resultStatusBadge");

    QHBoxLayout* filterLayout = new QHBoxLayout;
    filterLayout->setSpacing(10);

    QLabel* searchLabel = new QLabel("Search pathway:");
    searchLabel->setObjectName("fieldLabel");

    pathwaySearchBox = new QLineEdit;
    pathwaySearchBox->setPlaceholderText("Example: cell cycle");
    pathwaySearchBox->setClearButtonEnabled(true);

    QLabel* categoryLabel = new QLabel("Category:");
    categoryLabel->setObjectName("fieldLabel");

    pathwayCategoryBox = new QComboBox;
    pathwayCategoryBox->addItems(
        {
            "All categories",
            "GO Biological Process",
            "Signalling Pathway",
            "Stress Response",
            "Disease Process"
        }
    );

    QLabel* thresholdLabel = new QLabel("Adjusted p-value <=");
    thresholdLabel->setObjectName("fieldLabel");

    enrichmentPThresholdBox = new QDoubleSpinBox;
    enrichmentPThresholdBox->setRange(0.000001, 1.0);
    enrichmentPThresholdBox->setDecimals(6);
    enrichmentPThresholdBox->setSingleStep(0.01);
    enrichmentPThresholdBox->setValue(1.0);

    QLabel* maximumLabel = new QLabel("Display:");
    maximumLabel->setObjectName("fieldLabel");

    maximumPathwaysBox = new QComboBox;
    maximumPathwaysBox->addItem("All pathways", 0);
    maximumPathwaysBox->addItem("Top 5", 5);
    maximumPathwaysBox->addItem("Top 10", 10);
    maximumPathwaysBox->addItem("Top 20", 20);

    QPushButton* resetButton = new QPushButton("Reset Filters");
    resetButton->setObjectName("compactActionButton");

    QVBoxLayout* pathwayField = new QVBoxLayout;
    pathwayField->setSpacing(4);
    pathwayField->addWidget(searchLabel);
    pathwayField->addWidget(pathwaySearchBox);

    QVBoxLayout* categoryField = new QVBoxLayout;
    categoryField->setSpacing(4);
    categoryField->addWidget(categoryLabel);
    categoryField->addWidget(pathwayCategoryBox);

    QVBoxLayout* thresholdField = new QVBoxLayout;
    thresholdField->setSpacing(4);
    thresholdField->addWidget(thresholdLabel);
    thresholdField->addWidget(enrichmentPThresholdBox);

    QVBoxLayout* pathwayDisplayField = new QVBoxLayout;
    pathwayDisplayField->setSpacing(4);
    pathwayDisplayField->addWidget(maximumLabel);
    pathwayDisplayField->addWidget(maximumPathwaysBox);

    filterLayout->addLayout(pathwayField, 2);
    filterLayout->addLayout(categoryField, 1);
    filterLayout->addLayout(thresholdField, 1);
    filterLayout->addLayout(pathwayDisplayField, 1);
    filterLayout->addWidget(resetButton, 0, Qt::AlignBottom);

    filterCardLayout->addWidget(filterTitle);
    filterCardLayout->addWidget(filterDescription);
    filterCardLayout->addLayout(filterLayout);

    QFrame* resultCard = new QFrame;
    resultCard->setObjectName("analysisCard");

    QVBoxLayout* resultCardLayout = new QVBoxLayout(resultCard);
    resultCardLayout->setContentsMargins(18, 12, 18, 14);
    resultCardLayout->setSpacing(7);

    QHBoxLayout* resultHeader = new QHBoxLayout;
    QVBoxLayout* resultTitleLayout = new QVBoxLayout;
    resultTitleLayout->setSpacing(2);

    QLabel* resultTitle = new QLabel("Enriched pathways");
    resultTitle->setObjectName("panelTitle");

    QLabel* resultDescription = new QLabel(
        "Hypergeometric overrepresentation with Benjamini-Hochberg multiple-testing correction."
    );
    resultDescription->setObjectName("panelDescription");

    resultTitleLayout->addWidget(resultTitle);
    resultTitleLayout->addWidget(resultDescription);

    enrichmentResultsTable = new QTableWidget;
    enrichmentResultsTable->setColumnCount(8);
    enrichmentResultsTable->setHorizontalHeaderLabels(
        {
            "Pathway ID",
            "Pathway",
            "Category",
            "Overlap",
            "Fold Enrichment",
            "P-value",
            "Adjusted P-value",
            "Contributing Genes"
        }
    );
    styleDashboardTable(enrichmentResultsTable);
    enrichmentResultsTable->horizontalHeader()->setSectionResizeMode(
        QHeaderView::ResizeToContents
    );
    enrichmentResultsTable->horizontalHeader()->setStretchLastSection(true);

    enrichmentBarChart = new EnrichmentBarChartWidget;
    enrichmentBarChart->setMinimumWidth(300);
    enrichmentBarChart->setMaximumWidth(360);

    QPushButton* exportButton = new QPushButton(
        "Export results"
    );

    QPushButton* exportChartButton = new QPushButton(
        "Export chart PNG"
    );

    exportButton->setProperty("primary", true);
    exportChartButton->setObjectName("compactActionButton");
    exportButton->setCursor(Qt::PointingHandCursor);
    exportChartButton->setCursor(Qt::PointingHandCursor);

    resultHeader->addLayout(resultTitleLayout, 1);
    resultHeader->addWidget(exportButton, 0, Qt::AlignTop);
    resultHeader->addWidget(exportChartButton, 0, Qt::AlignTop);

    QHBoxLayout* resultContentLayout = new QHBoxLayout;
    resultContentLayout->setSpacing(10);
    resultContentLayout->addWidget(enrichmentResultsTable, 1);
    resultContentLayout->addWidget(enrichmentBarChart);

    resultCardLayout->addLayout(resultHeader);
    resultCardLayout->addWidget(enrichmentSummaryLabel);
    resultCardLayout->addLayout(resultContentLayout, 1);

    layout->addWidget(filterCard);
    layout->addWidget(resultCard, 1);

    connect(
        pathwaySearchBox,
        &QLineEdit::textChanged,
        this,
        [this]() { applyEnrichmentFilters(); }
    );

    connect(
        pathwayCategoryBox,
        &QComboBox::currentIndexChanged,
        this,
        [this]() { applyEnrichmentFilters(); }
    );

    connect(
        enrichmentPThresholdBox,
        &QDoubleSpinBox::valueChanged,
        this,
        [this]() { applyEnrichmentFilters(); }
    );

    connect(
        maximumPathwaysBox,
        &QComboBox::currentIndexChanged,
        this,
        [this]() { applyEnrichmentFilters(); }
    );

    connect(
        resetButton,
        &QPushButton::clicked,
        this,
        &MainWindow::resetEnrichmentFilters
    );

    connect(
        exportButton,
        &QPushButton::clicked,
        this,
        &MainWindow::exportEnrichmentResults
    );

    connect(
        exportChartButton,
        &QPushButton::clicked,
        this,
        [this]()
        {
            exportWidgetImage(
                enrichmentBarChart,
                "pathway_enrichment_chart.png",
                "Export Pathway-Enrichment Chart"
            );
        }
    );

    return page;
}

QWidget* MainWindow::createWorkflowBuilderPage()
{
    QWidget* page = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(page);

    layout->setContentsMargins(28, 16, 28, 18);
    layout->setSpacing(10);

    QFrame* controlCard = new QFrame;
    controlCard->setObjectName("analysisCard");

    QVBoxLayout* controlLayout = new QVBoxLayout(controlCard);
    controlLayout->setContentsMargins(18, 12, 18, 12);
    controlLayout->setSpacing(7);

    QLabel* controlTitle = new QLabel("Workflow controls");
    controlTitle->setObjectName("panelTitle");

    QLabel* controlDescription = new QLabel(
        "Load a template, add or connect steps, validate dependencies and export the resulting workflow."
    );
    controlDescription->setObjectName("panelDescription");

    workflowTemplateBox = new QComboBox;
    workflowTemplateBox->addItem("Gene Expression Workflow");
    workflowTemplateBox->addItem("Phylogenetic Workflow");

    QPushButton* loadTemplateButton = new QPushButton("Load Template");
    loadTemplateButton->setProperty("primary", true);

    workflowNodeTypeBox = new QComboBox;
    workflowNodeTypeBox->addItems(
        {"Input Node", "Analysis Node", "Visualization Node", "Output Node"}
    );

    QPushButton* addNodeButton = new QPushButton("Add Step");
    QPushButton* connectButton = new QPushButton("Connect Nodes");
    QPushButton* removeButton = new QPushButton("Remove Selected");
    QPushButton* validateButton = new QPushButton("Validate Workflow");
    QPushButton* exportWorkflowButton = new QPushButton("Export Workflow PNG");

    for (QPushButton* button :
         {addNodeButton, connectButton, removeButton, validateButton,
          exportWorkflowButton})
    {
        button->setObjectName("compactActionButton");
        button->setCursor(Qt::PointingHandCursor);
    }

    loadTemplateButton->setCursor(Qt::PointingHandCursor);

    QHBoxLayout* templateLayout = new QHBoxLayout;
    QLabel* templateLabel = new QLabel("Template");
    templateLabel->setObjectName("fieldLabel");
    QLabel* nodeTypeLabel = new QLabel("New step type");
    nodeTypeLabel->setObjectName("fieldLabel");

    templateLayout->addWidget(templateLabel);
    templateLayout->addWidget(workflowTemplateBox, 1);
    templateLayout->addWidget(loadTemplateButton);
    templateLayout->addSpacing(15);
    templateLayout->addWidget(nodeTypeLabel);
    templateLayout->addWidget(workflowNodeTypeBox);
    templateLayout->addWidget(addNodeButton);

    QHBoxLayout* editLayout = new QHBoxLayout;
    editLayout->addWidget(connectButton);
    editLayout->addWidget(removeButton);
    editLayout->addWidget(validateButton);
    editLayout->addWidget(exportWorkflowButton);
    editLayout->addStretch();

    controlLayout->addWidget(controlTitle);
    controlLayout->addWidget(controlDescription);
    controlLayout->addLayout(templateLayout);
    controlLayout->addLayout(editLayout);

    workflowCanvas = new WorkflowCanvasWidget;

    QFrame* canvasCard = new QFrame;
    canvasCard->setObjectName("analysisCard");

    QVBoxLayout* canvasLayout = new QVBoxLayout(canvasCard);
    canvasLayout->setContentsMargins(14, 12, 14, 14);
    canvasLayout->setSpacing(7);

    QLabel* canvasTitle = new QLabel("Workflow canvas");
    canvasTitle->setObjectName("panelTitle");

    QLabel* canvasHint = new QLabel(
        "Drag nodes to reposition them; select two nodes when creating a dependency."
    );
    canvasHint->setObjectName("panelHint");

    workflowCanvas->setMinimumSize(1020, 360);

    QScrollArea* workflowScrollArea = new QScrollArea;
    workflowScrollArea->setWidget(workflowCanvas);
    workflowScrollArea->setWidgetResizable(false);
    workflowScrollArea->setFrameShape(QFrame::NoFrame);
    workflowScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    workflowScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    workflowScrollArea->setStyleSheet(
        "QScrollArea { background-color: #F8FAFC; border: none; }"
        "QScrollArea > QWidget > QWidget { background-color: #F8FAFC; }"
    );

    canvasLayout->addWidget(canvasTitle);
    canvasLayout->addWidget(canvasHint);
    canvasLayout->addWidget(workflowScrollArea, 1);

    QFrame* inspectorCard = new QFrame;
    inspectorCard->setObjectName("analysisCard");
    inspectorCard->setMinimumHeight(82);
    inspectorCard->setMaximumHeight(105);

    QHBoxLayout* inspectorLayout = new QHBoxLayout(inspectorCard);
    inspectorLayout->setContentsMargins(16, 10, 16, 10);
    inspectorLayout->setSpacing(12);

    QLabel* inspectorTitle = new QLabel("Selected step");
    inspectorTitle->setObjectName("panelTitle");

    workflowSelectionLabel = new QLabel(
        "Select a workflow node to inspect its biological purpose."
    );
    workflowSelectionLabel->setWordWrap(true);
    workflowSelectionLabel->setObjectName("panelHint");

    workflowValidationLabel = new QLabel(
        "Load or edit a workflow, then validate its dependencies."
    );
    workflowValidationLabel->setWordWrap(true);
    workflowValidationLabel->setObjectName("resultStatusBadge");
    workflowValidationLabel->setMaximumWidth(320);

    openSelectedWorkflowStepButton = new QPushButton(
        "Open selected analysis  →"
    );
    openSelectedWorkflowStepButton->setProperty("primary", true);
    openSelectedWorkflowStepButton->setCursor(Qt::PointingHandCursor);
    openSelectedWorkflowStepButton->setEnabled(false);

    QVBoxLayout* selectionLayout = new QVBoxLayout;
    selectionLayout->setSpacing(3);
    selectionLayout->addWidget(inspectorTitle);
    selectionLayout->addWidget(workflowSelectionLabel);

    inspectorLayout->addLayout(selectionLayout, 1);
    inspectorLayout->addWidget(workflowValidationLabel);
    inspectorLayout->addWidget(openSelectedWorkflowStepButton);

    QVBoxLayout* workspaceLayout = new QVBoxLayout;
    workspaceLayout->setSpacing(10);
    workspaceLayout->addWidget(canvasCard, 1);
    workspaceLayout->addWidget(inspectorCard);

    layout->addWidget(controlCard);
    layout->addLayout(workspaceLayout, 1);

    workflowCanvas->setSelectionChangedCallback(
        [this](const WorkflowNode* node)
        {
            if (node == nullptr)
            {
                workflowSelectionLabel->setText(
                    "Select a workflow node to inspect its biological purpose."
                );
                openSelectedWorkflowStepButton->setEnabled(false);
                return;
            }

            workflowSelectionLabel->setText(
                QString("%1 | %2 | Status: %3\n%4")
                    .arg(QString::fromStdString(node->getName()))
                    .arg(QString::fromStdString(node->getCategoryName()))
                    .arg(QString::fromStdString(node->getStateName()))
                    .arg(QString::fromStdString(node->getDescription()))
            );
            openSelectedWorkflowStepButton->setEnabled(true);
        }
    );

    workflowCanvas->setGraphChangedCallback(
        [this]()
        {
            workflowValidationLabel->setText(
                "Workflow modified — validate before using it."
            );
            workflowValidationLabel->setStyleSheet(
                "font-weight: 700; color: #FBBF24;"
            );
        }
    );

    workflowCanvas->setMessageCallback(
        [this](const QString& message)
        {
            workflowValidationLabel->setText(message);
            workflowValidationLabel->setStyleSheet(
                "font-weight: 700; color: #93C5FD;"
            );
        }
    );

    connect(
        loadTemplateButton,
        &QPushButton::clicked,
        this,
        &MainWindow::loadWorkflowTemplate
    );

    connect(
        addNodeButton,
        &QPushButton::clicked,
        this,
        [this]()
        {
            WorkflowNodeKind kind = WorkflowNodeKind::Processing;

            if (workflowNodeTypeBox->currentIndex() == 0)
            {
                kind = WorkflowNodeKind::Input;
            }
            else if (workflowNodeTypeBox->currentIndex() == 2)
            {
                kind = WorkflowNodeKind::Visualization;
            }
            else if (workflowNodeTypeBox->currentIndex() == 3)
            {
                kind = WorkflowNodeKind::Output;
            }

            workflowCanvas->addNode(kind);
        }
    );

    connect(
        connectButton,
        &QPushButton::clicked,
        this,
        [this]() { workflowCanvas->beginConnectionMode(); }
    );

    connect(
        removeButton,
        &QPushButton::clicked,
        this,
        [this]() { workflowCanvas->removeSelectedNode(); }
    );

    connect(
        validateButton,
        &QPushButton::clicked,
        this,
        [this]()
        {
            std::string validationMessage;
            bool valid = workflowCanvas->getGraph().isValid(
                validationMessage
            );

            workflowValidationLabel->setText(
                QString::fromStdString(validationMessage)
            );
            workflowValidationLabel->setStyleSheet(
                valid
                    ? "font-weight: 700; color: #6EE7B7;"
                    : "font-weight: 700; color: #FCA5A5;"
            );
        }
    );

    connect(
        exportWorkflowButton,
        &QPushButton::clicked,
        this,
        [this]()
        {
            exportWidgetImage(
                workflowCanvas,
                "bioinformatics_workflow.png",
                "Export Workflow Diagram"
            );
        }
    );

    connect(
        openSelectedWorkflowStepButton,
        &QPushButton::clicked,
        this,
        &MainWindow::openSelectedWorkflowStep
    );

    loadWorkflowTemplate();
    return page;
}

QWidget* MainWindow::createProjectHistoryPage()
{
    QWidget* page = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(page);

    layout->setContentsMargins(28, 16, 28, 18);

    QFrame* historyCard = new QFrame;
    historyCard->setObjectName("analysisCard");

    QVBoxLayout* cardLayout = new QVBoxLayout(historyCard);
    cardLayout->setContentsMargins(18, 14, 18, 16);
    cardLayout->setSpacing(9);

    QHBoxLayout* headerLayout = new QHBoxLayout;
    QVBoxLayout* titleLayout = new QVBoxLayout;
    titleLayout->setSpacing(2);

    QLabel* title = new QLabel("Analysis activity");
    title->setObjectName("panelTitle");

    QLabel* description = new QLabel(
        "Chronological record of imported datasets, configuration changes and completed analyses in this project."
    );
    description->setObjectName("panelDescription");
    description->setWordWrap(true);

    titleLayout->addWidget(title);
    titleLayout->addWidget(description);

    projectHistorySummaryLabel = new QLabel("No history entries yet.");
    projectHistorySummaryLabel->setAlignment(Qt::AlignCenter);
    projectHistorySummaryLabel->setObjectName("resultStatusBadge");

    projectHistoryTable = new QTableWidget;
    projectHistoryTable->setColumnCount(5);
    projectHistoryTable->setHorizontalHeaderLabels(
        {"Date and Time", "Workspace", "Action", "Status", "Details"}
    );
    styleDashboardTable(projectHistoryTable);
    projectHistoryTable->horizontalHeader()->setSectionResizeMode(
        0, QHeaderView::ResizeToContents
    );
    projectHistoryTable->horizontalHeader()->setSectionResizeMode(
        1, QHeaderView::ResizeToContents
    );
    projectHistoryTable->horizontalHeader()->setSectionResizeMode(
        2, QHeaderView::ResizeToContents
    );
    projectHistoryTable->horizontalHeader()->setSectionResizeMode(
        3, QHeaderView::ResizeToContents
    );
    projectHistoryTable->horizontalHeader()->setSectionResizeMode(
        4, QHeaderView::Stretch
    );

    QPushButton* saveButton = new QPushButton("Save project");
    saveButton->setProperty("primary", true);
    saveButton->setCursor(Qt::PointingHandCursor);

    headerLayout->addLayout(titleLayout, 1);
    headerLayout->addWidget(projectHistorySummaryLabel, 0, Qt::AlignTop);
    headerLayout->addWidget(saveButton, 0, Qt::AlignTop);

    cardLayout->addLayout(headerLayout);
    cardLayout->addWidget(projectHistoryTable, 1);
    layout->addWidget(historyCard, 1);

    connect(
        saveButton,
        &QPushButton::clicked,
        this,
        &MainWindow::saveProject
    );
    return page;
}

void MainWindow::selectPhylogeneticWorkspace()
{
    currentProject.setWorkspaceType(
        WorkspaceType::PhylogeneticAnalysis
    );

    statusLabel->setText(
        "Selected workspace: "
        + QString::fromStdString(
            currentProject.getWorkspaceName()
        )
    );

    pages->setCurrentIndex(
        PhylogeneticSetupPage
    );
}

void MainWindow::selectGeneExpressionWorkspace()
{
    currentProject.setWorkspaceType(
        WorkspaceType::GeneExpressionAnalysis
    );

    statusLabel->setText(
        "Selected workspace: "
        + QString::fromStdString(
            currentProject.getWorkspaceName()
        )
    );

    pages->setCurrentIndex(
        GeneExpressionPage
    );
}

void MainWindow::openWorkflowBuilderPage()
{
    refreshWorkflowNodeStates();
    pages->setCurrentIndex(WorkflowBuilderPage);
}

void MainWindow::openProjectHistoryPage()
{
    refreshProjectHistoryTable();
    pages->setCurrentIndex(ProjectHistoryPage);
}

void MainWindow::loadWorkflowTemplate()
{
    WorkflowTemplateType type = workflowTemplateBox->currentIndex() == 1
        ? WorkflowTemplateType::Phylogenetic
        : WorkflowTemplateType::GeneExpression;

    workflowCanvas->setGraph(WorkflowTemplateFactory::create(type));
    refreshWorkflowNodeStates();

    workflowValidationLabel->setText(
        "Template loaded. Drag nodes to rearrange them or validate the workflow."
    );
    workflowValidationLabel->setStyleSheet(
        "font-weight: 700; color: #6EE7B7;"
    );
}

void MainWindow::refreshWorkflowNodeStates()
{
    if (workflowCanvas == nullptr || workflowTemplateBox == nullptr)
    {
        return;
    }

    bool expressionWorkflow = workflowTemplateBox->currentIndex() == 0;

    for (const auto& nodePointer : workflowCanvas->getGraph().getNodes())
    {
        WorkflowNode* node = nodePointer.get();
        const std::string& name = node->getName();
        WorkflowNodeState state = WorkflowNodeState::Pending;

        if (expressionWorkflow)
        {
            if (name == "Import Expression Data")
            {
                state = expressionDataset
                    ? WorkflowNodeState::Completed
                    : WorkflowNodeState::Ready;
            }
            else if (name == "Data Quality")
            {
                state = expressionDataset
                    ? WorkflowNodeState::Completed
                    : WorkflowNodeState::Blocked;
            }
            else if (name == "Groups and Normalization")
            {
                state = !expressionResults.empty()
                    ? WorkflowNodeState::Completed
                    : expressionDataset
                        ? WorkflowNodeState::Ready
                        : WorkflowNodeState::Blocked;
            }
            else if (name == "Differential Expression")
            {
                state = !expressionResults.empty()
                    ? WorkflowNodeState::Completed
                    : expressionDataset
                        ? WorkflowNodeState::Ready
                        : WorkflowNodeState::Blocked;
            }
            else if (name == "Plots and Exploration")
            {
                state = !expressionResults.empty()
                    ? WorkflowNodeState::Completed
                    : WorkflowNodeState::Blocked;
            }
            else if (name == "Pathway Enrichment")
            {
                state = !enrichmentResults.empty()
                    ? WorkflowNodeState::Completed
                    : !expressionResults.empty()
                        ? WorkflowNodeState::Ready
                        : WorkflowNodeState::Blocked;
            }
            else if (name == "Export and Report")
            {
                state = !expressionResults.empty()
                    ? WorkflowNodeState::Ready
                    : WorkflowNodeState::Blocked;
            }
        }
        else
        {
            if (name == "Import FASTA")
            {
                state = loadedSequences.empty()
                    ? WorkflowNodeState::Ready
                    : WorkflowNodeState::Completed;
            }
            else if (name == "Sequence Quality")
            {
                state = loadedSequences.empty()
                    ? WorkflowNodeState::Blocked
                    : WorkflowNodeState::Completed;
            }
            else if (name == "Distance Matrix")
            {
                state = currentDistanceMatrix.size() > 0
                    || treeDistanceMatrix.size() > 0
                    ? WorkflowNodeState::Completed
                    : loadedSequences.size() >= 2
                        ? WorkflowNodeState::Ready
                        : WorkflowNodeState::Blocked;
            }
            else if (name == "UPGMA Tree"
                     || name == "Tree Visualization")
            {
                state = !currentTree.isEmpty()
                    ? WorkflowNodeState::Completed
                    : loadedSequences.size() >= 2
                        ? WorkflowNodeState::Ready
                        : WorkflowNodeState::Blocked;
            }
            else if (name == "Export and Report")
            {
                state = currentDistanceMatrix.size() > 0
                    || !currentTree.isEmpty()
                    ? WorkflowNodeState::Ready
                    : WorkflowNodeState::Blocked;
            }
        }

        node->setState(state);
    }

    workflowCanvas->update();
}

void MainWindow::openSelectedWorkflowStep()
{
    const WorkflowNode* node = workflowCanvas->getSelectedNode();

    if (node == nullptr)
    {
        return;
    }

    std::string name = node->getName();
    bool expressionWorkflow = workflowTemplateBox->currentIndex() == 0;

    if (expressionWorkflow)
    {
        if (name == "Import Expression Data")
        {
            pages->setCurrentIndex(GeneExpressionPage);
        }
        else if (name == "Data Quality")
        {
            openExpressionQualityPage();
        }
        else if (name == "Groups and Normalization"
                 || name == "Differential Expression")
        {
            openExpressionConfigurationPage();
        }
        else if (name == "Plots and Exploration"
                 || name == "Export and Report")
        {
            if (expressionResults.empty())
            {
                openExpressionConfigurationPage();
            }
            else
            {
                pages->setCurrentIndex(ExpressionResultsPage);
            }
        }
        else if (name == "Pathway Enrichment")
        {
            openExpressionEnrichmentPage();
        }
        else
        {
            QMessageBox::information(
                this,
                "Custom Workflow Step",
                "This custom node demonstrates editable workflow design. "
                "Connect it to a concrete analysis class to make it executable."
            );
        }
    }
    else
    {
        if (name == "Import FASTA")
        {
            pages->setCurrentIndex(PhylogeneticSetupPage);
        }
        else if (name == "Sequence Quality")
        {
            openPhylogeneticQualityPage();
        }
        else if (name == "Distance Matrix")
        {
            openDistanceMatrixPage();
        }
        else if (name == "UPGMA Tree"
                 || name == "Tree Visualization"
                 || name == "Export and Report")
        {
            openPhylogeneticTreePage();
        }
        else
        {
            QMessageBox::information(
                this,
                "Custom Workflow Step",
                "This custom node is not yet connected to an executable "
                "phylogenetic analysis component."
            );
        }
    }
}

void MainWindow::openDistanceMatrixPage()
{
    pages->setCurrentIndex(
        DistanceMatrixPage
    );
}

void MainWindow::openPhylogeneticHeatmapPage()
{
    if (currentDistanceMatrix.size() == 0)
    {
        QMessageBox::information(
            this,
            "Generate Matrix First",
            "Generate the distance matrix before opening its heatmap."
        );
        return;
    }

    pages->setCurrentIndex(PhylogeneticHeatmapPage);
}

void MainWindow::openPhylogeneticTreePage()
{
    pages->setCurrentIndex(
        PhylogeneticTreePage
    );
}

void MainWindow::openExpressionConfigurationPage()
{
    if (!expressionDataset)
    {
        QMessageBox::warning(
            this,
            "No Expression Dataset",
            "Please import a valid expression dataset first."
        );
        return;
    }

    populateSampleGroupingTable();
    pages->setCurrentIndex(ExpressionConfigurationPage);
}

void MainWindow::openExpressionVolcanoPage()
{
    if (classifiedExpressionResults.empty())
    {
        QMessageBox::warning(
            this,
            "No Analysis Results",
            "Run differential expression analysis before opening "
            "the volcano plot."
        );
        return;
    }

    volcanoPlotWidget->setResults(
        classifiedExpressionResults,
        adjustedPThresholdBox->value(),
        foldChangeThresholdBox->value()
    );
    pages->setCurrentIndex(ExpressionVolcanoPage);
}

void MainWindow::openExpressionHeatmapPage()
{
    if (!expressionDataset
        || filteredExpressionResults.empty()
        || currentNormalizedExpressionValues.empty())
    {
        QMessageBox::warning(
            this,
            "No Analysis Results",
            "Run differential expression analysis before opening "
            "the expression heatmap."
        );
        return;
    }

    try
    {
        constexpr std::size_t maximumDisplayedGenes = 50;

        expressionHeatmapWidget->setData(
            *expressionDataset,
            currentNormalizedExpressionValues,
            filteredExpressionResults,
            sampleGrouping,
            maximumDisplayedGenes
        );

        fitEmbeddedHeatmapTable(expressionHeatmapWidget);

        std::size_t displayedGenes = std::min(
            maximumDisplayedGenes,
            filteredExpressionResults.size()
        );

        expressionHeatmapSummaryLabel->setText(
            QString(
                "Displaying %1 of %2 genes across %3 samples | "
                "Cell values are row Z-scores"
            )
            .arg(static_cast<qulonglong>(displayedGenes))
            .arg(static_cast<qulonglong>(
                expressionDataset->getGeneCount()
            ))
            .arg(static_cast<qulonglong>(
                expressionDataset->getSampleCount()
            ))
        );
        expressionHeatmapSummaryLabel->setStyleSheet(
            "font-weight: 700; color: #6EE7B7;"
        );

        pages->setCurrentIndex(ExpressionHeatmapPage);
    }
    catch (const std::exception& error)
    {
        QMessageBox::critical(
            this,
            "Heatmap Error",
            QString::fromStdString(error.what())
        );
    }
}

void MainWindow::openExpressionPCAPage()
{
    if (!expressionDataset
        || currentNormalizedExpressionValues.empty())
    {
        QMessageBox::warning(
            this,
            "No Analysis Results",
            "Run differential expression analysis before opening "
            "the PCA plot."
        );
        return;
    }

    try
    {
        PCAAnalyzer analyzer;
        PCAResult result = analyzer.analyze(
            *expressionDataset,
            currentNormalizedExpressionValues
        );

        pcaPlotWidget->setResult(result, sampleGrouping);

        pcaSummaryLabel->setText(
            QString(
                "%1 samples analyzed | PC1 explains %2% | "
                "PC2 explains %3% | Combined: %4%"
            )
            .arg(static_cast<qulonglong>(result.getSampleCount()))
            .arg(result.getPC1ExplainedVariance(), 0, 'f', 1)
            .arg(result.getPC2ExplainedVariance(), 0, 'f', 1)
            .arg(
                result.getPC1ExplainedVariance()
                + result.getPC2ExplainedVariance(),
                0,
                'f',
                1
            )
        );
        pcaSummaryLabel->setStyleSheet(
            "font-weight: 700; color: #6EE7B7;"
        );

        pages->setCurrentIndex(ExpressionPCAPage);
    }
    catch (const std::exception& error)
    {
        QMessageBox::critical(
            this,
            "PCA Error",
            QString::fromStdString(error.what())
        );
    }
}

void MainWindow::openPhylogeneticQualityPage()
{
    if (loadedSequences.empty())
    {
        QMessageBox::warning(
            this,
            "No FASTA Data",
            "Import valid FASTA sequences before opening quality control."
        );
        return;
    }

    std::vector<std::string> sourceFiles;
    sourceFiles.reserve(
        static_cast<std::size_t>(selectedFastaFiles.size())
    );

    for (const QString& filePath : selectedFastaFiles)
    {
        sourceFiles.push_back(filePath.toStdString());
    }

    SequenceQualityAnalyzer analyzer(
        loadedSequences,
        sourceFiles
    );

    QualityReport report = analyzer.analyze();
    populateQualityReportTable(
        phylogeneticQualityTable,
        phylogeneticQualitySummaryLabel,
        report
    );

    recordHistory(
        "Phylogenetic Analysis",
        "Sequence Quality Control",
        "Completed",
        QString("Overall quality status: %1.")
            .arg(QString::fromStdString(
                QualityReport::statusName(report.getOverallStatus())
            ))
    );

    pages->setCurrentIndex(PhylogeneticQualityPage);
}

void MainWindow::openExpressionQualityPage()
{
    if (!expressionDataset)
    {
        QMessageBox::warning(
            this,
            "No Expression Data",
            "Import a valid expression dataset before opening "
            "quality control."
        );
        return;
    }

    ExpressionQualityAnalyzer analyzer(
        *expressionDataset,
        sampleGrouping
    );

    QualityReport report = analyzer.analyze();
    populateQualityReportTable(
        expressionQualityTable,
        expressionQualitySummaryLabel,
        report
    );

    recordHistory(
        "Gene Expression Analysis",
        "Expression Quality Control",
        "Completed",
        QString("Overall quality status: %1.")
            .arg(QString::fromStdString(
                QualityReport::statusName(report.getOverallStatus())
            ))
    );

    pages->setCurrentIndex(ExpressionQualityPage);
}

void MainWindow::openExpressionEnrichmentPage()
{
    if (!expressionDataset || classifiedExpressionResults.empty())
    {
        QMessageBox::warning(
            this,
            "No Differential Expression Results",
            "Run differential-expression analysis before pathway enrichment."
        );
        return;
    }

    runFunctionalEnrichmentAnalysis();
}

void MainWindow::runFunctionalEnrichmentAnalysis()
{
    std::vector<std::string> significantGenes;

    for (const DifferentialExpressionResult& result :
         classifiedExpressionResults)
    {
        if (result.getRegulationStatus()
            != RegulationStatus::NotSignificant)
        {
            significantGenes.push_back(result.getGeneName());
        }
    }

    if (significantGenes.empty())
    {
        QMessageBox::warning(
            this,
            "No Significant Genes",
            "No genes meet the current differential-expression thresholds. "
            "Adjust the p-value or fold-change filters and try again."
        );
        return;
    }

    try
    {
        BuiltInPathwayDatabase database;
        HypergeometricEnrichmentAnalyzer analyzer;

        enrichmentResults = analyzer.analyze(
            significantGenes,
            expressionDataset->getGeneNames(),
            database.getPathways()
        );

        if (enrichmentResults.empty())
        {
            QMessageBox::information(
                this,
                "No Pathway Matches",
                "The significant genes did not overlap the built-in teaching "
                "pathway database. The analysis itself completed correctly."
            );
        }

        resetEnrichmentFilters();
        pages->setCurrentIndex(ExpressionEnrichmentPage);

        recordHistory(
            "Gene Expression Analysis",
            "Functional Enrichment",
            "Completed",
            QString("Tested %1 significant gene(s); returned %2 pathway result(s).")
                .arg(static_cast<qulonglong>(significantGenes.size()))
                .arg(static_cast<qulonglong>(enrichmentResults.size()))
        );
    }
    catch (const std::exception& error)
    {
        QMessageBox::critical(
            this,
            "Enrichment Analysis Error",
            QString::fromStdString(error.what())
        );
    }
}

void MainWindow::applyEnrichmentFilters()
{
    if (enrichmentResultsTable == nullptr)
    {
        return;
    }

    filteredEnrichmentResults.clear();

    QString query = pathwaySearchBox->text().trimmed();
    QString category = pathwayCategoryBox->currentText();
    double maximumAdjustedP = enrichmentPThresholdBox->value();
    std::size_t maximumResults = static_cast<std::size_t>(
        maximumPathwaysBox->currentData().toInt()
    );

    for (const EnrichmentResult& result : enrichmentResults)
    {
        QString pathwayName = QString::fromStdString(
            result.getPathwayName()
        );
        QString pathwayId = QString::fromStdString(
            result.getPathwayId()
        );
        QString geneList = QString::fromStdString(
            result.getOverlappingGeneList()
        );

        bool matchesQuery = query.isEmpty()
            || pathwayName.contains(query, Qt::CaseInsensitive)
            || pathwayId.contains(query, Qt::CaseInsensitive)
            || geneList.contains(query, Qt::CaseInsensitive);

        bool matchesCategory = category == "All categories"
            || QString::fromStdString(result.getCategory()) == category;

        if (matchesQuery
            && matchesCategory
            && result.getAdjustedPValue() <= maximumAdjustedP)
        {
            filteredEnrichmentResults.push_back(result);
        }
    }

    if (maximumResults > 0
        && filteredEnrichmentResults.size() > maximumResults)
    {
        filteredEnrichmentResults.erase(
            filteredEnrichmentResults.begin()
                + static_cast<std::ptrdiff_t>(maximumResults),
            filteredEnrichmentResults.end()
        );
    }

    populateEnrichmentResultsTable();
}

void MainWindow::resetEnrichmentFilters()
{
    pathwaySearchBox->clear();
    pathwayCategoryBox->setCurrentIndex(0);
    enrichmentPThresholdBox->setValue(1.0);
    maximumPathwaysBox->setCurrentIndex(0);

    applyEnrichmentFilters();
}

void MainWindow::populateEnrichmentResultsTable()
{
    enrichmentResultsTable->setSortingEnabled(false);
    enrichmentResultsTable->clearContents();
    enrichmentResultsTable->setRowCount(
        static_cast<int>(filteredEnrichmentResults.size())
    );

    for (std::size_t row = 0;
         row < filteredEnrichmentResults.size();
         ++row)
    {
        const EnrichmentResult& result =
            filteredEnrichmentResults.at(row);
        int tableRow = static_cast<int>(row);

        QTableWidgetItem* pathwayItem = new QTableWidgetItem(
            QString::fromStdString(result.getPathwayName())
        );
        pathwayItem->setTextAlignment(Qt::AlignCenter);

        QTableWidgetItem* adjustedItem = new NumericTableWidgetItem(
            result.getAdjustedPValue(), 7
        );

        if (result.getAdjustedPValue() <= 0.05)
        {
            adjustedItem->setBackground(QColor("#CDEFD8"));
            adjustedItem->setForeground(QColor("#176B35"));
        }

        QTableWidgetItem* overlapItem = new QTableWidgetItem(
            QString("%1 / %2")
                .arg(static_cast<qulonglong>(result.getOverlapCount()))
                .arg(static_cast<qulonglong>(result.getPathwayGeneCount()))
        );
        overlapItem->setTextAlignment(Qt::AlignCenter);

        enrichmentResultsTable->setItem(
            tableRow, 0,
            new QTableWidgetItem(
                QString::fromStdString(result.getPathwayId())
            )
        );
        enrichmentResultsTable->setItem(tableRow, 1, pathwayItem);
        enrichmentResultsTable->setItem(
            tableRow, 2,
            new QTableWidgetItem(
                QString::fromStdString(result.getCategory())
            )
        );
        enrichmentResultsTable->setItem(tableRow, 3, overlapItem);
        enrichmentResultsTable->setItem(
            tableRow, 4,
            new NumericTableWidgetItem(result.getFoldEnrichment(), 6)
        );
        enrichmentResultsTable->setItem(
            tableRow, 5,
            new NumericTableWidgetItem(result.getPValue(), 7)
        );
        enrichmentResultsTable->setItem(tableRow, 6, adjustedItem);
        enrichmentResultsTable->setItem(
            tableRow, 7,
            new QTableWidgetItem(
                QString::fromStdString(result.getOverlappingGeneList())
            )
        );
    }

    enrichmentResultsTable->setSortingEnabled(true);
    enrichmentResultsTable->sortItems(6, Qt::AscendingOrder);
    enrichmentBarChart->setResults(filteredEnrichmentResults, 10);

    std::size_t significantPathways = 0;

    for (const EnrichmentResult& result : enrichmentResults)
    {
        significantPathways += result.getAdjustedPValue() <= 0.05 ? 1 : 0;
    }

    enrichmentSummaryLabel->setText(
        QString(
            "%1 pathway(s) tested | %2 significant | %3 currently shown | "
            "Green adjusted p-values are significant"
        )
        .arg(static_cast<qulonglong>(enrichmentResults.size()))
        .arg(static_cast<qulonglong>(significantPathways))
        .arg(static_cast<qulonglong>(filteredEnrichmentResults.size()))
    );

    enrichmentSummaryLabel->setStyleSheet(
        filteredEnrichmentResults.empty()
            ? "font-weight: 700; color: #FBBF24;"
            : "font-weight: 700; color: #6EE7B7;"
    );
}

void MainWindow::returnToDashboard()
{
    pages->setCurrentIndex(
        DashboardPage
    );
}

void MainWindow::returnToPhylogeneticSetup()
{
    pages->setCurrentIndex(
        PhylogeneticSetupPage
    );
}

void MainWindow::returnToGeneExpressionSetup()
{
    pages->setCurrentIndex(GeneExpressionPage);
}

void MainWindow::returnToExpressionResults()
{
    pages->setCurrentIndex(ExpressionResultsPage);
}

void MainWindow::importFastaFiles()
{
    QStringList filePaths =
        QFileDialog::getOpenFileNames(
            this,
            "Select FASTA Files",
            QString(),
            "FASTA Files (*.fasta *.fa *.fna *.faa);;"
            "All Files (*.*)"
        );

    if (filePaths.isEmpty())
    {
        return;
    }

    loadFastaFiles(filePaths, true);
}

void MainWindow::loadFastaFiles(
    const QStringList& filePaths,
    bool shouldRecordHistory
)
{

    FastaParser parser;

    selectedFastaFiles = filePaths;
    loadedSequences.clear();

    currentDistanceMatrix.clear();
    treeDistanceMatrix.clear();
    currentTree.clear();

    phylogeneticFileList->clear();

    distanceMatrixTable->clear();
    distanceHeatmapTable->clear();
    distanceMatrixTable->setRowCount(0);
    distanceMatrixTable->setColumnCount(0);
    distanceHeatmapTable->setRowCount(0);
    distanceHeatmapTable->setColumnCount(0);

    treeGraphic->clearTree();
    treeOutput->clear();

    openMatrixButton->setEnabled(false);
    openTreeButton->setEnabled(false);
    phylogeneticQualityButton->setEnabled(false);

    try
    {
        for (const QString& filePath :
             selectedFastaFiles)
        {
            QFileInfo fileInformation(
                filePath
            );

            phylogeneticFileList->addItem(
                "FILE: "
                + fileInformation.fileName()
            );

            auto parsedSequences =
                parser.parseFile(
                    filePath.toStdString()
                );

            for (auto& sequence :
                 parsedSequences)
            {
                QString information =
                    QString(
                        "    %1 | %2 | Length: %3"
                    )
                    .arg(
                        QString::fromStdString(
                            sequence->getIdentifier()
                        )
                    )
                    .arg(
                        QString::fromStdString(
                            sequence->getTypeName()
                        )
                    )
                    .arg(
                        static_cast<qulonglong>(
                            sequence->getLength()
                        )
                    );

                phylogeneticFileList->addItem(
                    information
                );

                loadedSequences.push_back(
                    std::move(sequence)
                );
            }
        }

        phylogeneticFileList->insertItem(
            0,
            QString(
                "Successfully loaded %1 "
                "biological sequence(s)"
            ).arg(
                static_cast<qulonglong>(
                    loadedSequences.size()
                )
            )
        );

        bool enoughSequences =
            loadedSequences.size() >= 2;

        openMatrixButton->setEnabled(
            enoughSequences
        );

        openTreeButton->setEnabled(
            enoughSequences
        );

        phylogeneticQualityButton->setEnabled(
            !loadedSequences.empty()
        );

        if (shouldRecordHistory)
        {
            recordHistory(
                "Phylogenetic Analysis",
                "Import FASTA",
                "Completed",
                QString("Loaded %1 sequence(s) from %2 file(s).")
                    .arg(static_cast<qulonglong>(loadedSequences.size()))
                    .arg(selectedFastaFiles.size())
            );
        }
    }
    catch (const std::exception& error)
    {
        loadedSequences.clear();

        phylogeneticFileList->clear();
        phylogeneticFileList->addItem(
            "FASTA import failed."
        );

        QMessageBox::critical(
            this,
            "FASTA Import Error",
            QString::fromStdString(
                error.what()
            )
        );
    }
}

std::unique_ptr<PairwiseAligner>
MainWindow::createAligner(
    const QString& methodName
) const
{
    if (methodName == "Hamming Distance")
    {
        return std::make_unique<
            HammingAligner
        >();
    }

    return std::make_unique<
        NeedlemanWunschAligner
    >();
}

void MainWindow::generateDistanceMatrix()
{
    if (loadedSequences.size() < 2)
    {
        QMessageBox::warning(
            this,
            "Insufficient Sequences",
            "Import at least two compatible sequences."
        );

        return;
    }

    auto aligner = createAligner(
        matrixAlignmentMethodBox->currentText()
    );

    try
    {
        currentDistanceMatrix.calculate(
            loadedSequences,
            *aligner
        );

        populateDistanceMatrixTable(
            currentDistanceMatrix
        );

        matrixStatusLabel->setText(
            matrixAlignmentMethodBox->currentText()
        );

        matrixStatusLabel->setStyleSheet(
            "font-weight: bold;"
            "color: #6EE7B7;"
        );

        recordHistory(
            "Phylogenetic Analysis",
            "Generate Distance Matrix",
            "Completed",
            QString("%1 sequence(s) analyzed using %2.")
                .arg(static_cast<qulonglong>(loadedSequences.size()))
                .arg(matrixAlignmentMethodBox->currentText())
        );
    }
    catch (const std::exception& error)
    {
        currentDistanceMatrix.clear();

        matrixStatusLabel->setText(
            "Distance matrix generation failed"
        );

        matrixStatusLabel->setStyleSheet(
            "font-weight: bold;"
            "color: #FCA5A5;"
        );

        QMessageBox::critical(
            this,
            "Distance Matrix Error",
            QString::fromStdString(
                error.what()
            )
        );
    }
}

void MainWindow::populateDistanceMatrixTable(
    const DistanceMatrix& matrix
)
{
    std::size_t matrixSize = matrix.size();
    const auto& matrixValues = matrix.getValues();

    double maximumDistance = 0.0;

    for (const auto& row : matrixValues)
    {
        for (double distance : row)
        {
            maximumDistance = std::max(
                maximumDistance,
                distance
            );
        }
    }

    distanceMatrixTable->clear();

    distanceMatrixTable->setRowCount(
        static_cast<int>(matrixSize)
    );

    distanceMatrixTable->setColumnCount(
        static_cast<int>(matrixSize)
    );

    distanceHeatmapTable->setRowCount(
        static_cast<int>(matrixSize)
    );

    distanceHeatmapTable->setColumnCount(
        static_cast<int>(matrixSize)
    );

    QStringList labels;

    for (const std::string& label :
         matrix.getLabels())
    {
        labels.append(
            QString::fromStdString(label)
        );
    }

    QStringList columnLabels;

    for (int index = 0; index < labels.size(); ++index)
    {
        columnLabels.append(QString("S%1").arg(index + 1));
    }

    distanceMatrixTable
        ->setHorizontalHeaderLabels(columnLabels);

    distanceMatrixTable
        ->setVerticalHeaderLabels(labels);

    distanceHeatmapTable
        ->setHorizontalHeaderLabels(columnLabels);

    distanceHeatmapTable
        ->setVerticalHeaderLabels(labels);

    for (int index = 0; index < labels.size(); ++index)
    {
        distanceMatrixTable->horizontalHeaderItem(index)->setToolTip(
            QString("S%1: %2").arg(index + 1).arg(labels.at(index))
        );

        distanceHeatmapTable->horizontalHeaderItem(index)->setToolTip(
            QString("S%1: %2").arg(index + 1).arg(labels.at(index))
        );
    }

    for (std::size_t row = 0;
         row < matrixSize;
         ++row)
    {
        for (std::size_t column = 0;
             column < matrixSize;
             ++column)
        {
            double distance =
                matrix.getDistance(
                    row,
                    column
                );

            double normalizedDistance =
                maximumDistance > 0.0
                    ? distance / maximumDistance
                    : 0.0;

            normalizedDistance = std::clamp(
                normalizedDistance,
                0.0,
                1.0
            );

            double colorHue =
                (1.0 - normalizedDistance)
                * 0.33;

            QColor cellColor =
                QColor::fromHsvF(
                    colorHue,
                    0.45,
                    1.0
                );

            if (row == column)
            {
                cellColor =
                    QColor("#B7E4C7");
            }

            QTableWidgetItem* item =
                new QTableWidgetItem(
                    QString::number(
                        distance,
                        'f',
                        4
                    )
                );

            item->setTextAlignment(
                Qt::AlignCenter
            );

            item->setBackground(cellColor);
            item->setForeground(
                QColor("#152536")
            );

            item->setToolTip(
                QString(
                    "%1 versus %2\nDistance: %3"
                )
                .arg(
                    labels.at(
                        static_cast<int>(row)
                    )
                )
                .arg(
                    labels.at(
                        static_cast<int>(column)
                    )
                )
                .arg(
                    distance,
                    0,
                    'f',
                    6
                )
            );

            distanceMatrixTable->setItem(
                static_cast<int>(row),
                static_cast<int>(column),
                item
            );

            QTableWidgetItem* heatmapItem =
                new QTableWidgetItem(
                    QString::number(distance, 'f', 4)
                );

            heatmapItem->setBackground(cellColor);
            heatmapItem->setForeground(QColor("#152536"));
            heatmapItem->setTextAlignment(Qt::AlignCenter);
            heatmapItem->setToolTip(item->toolTip());

            distanceHeatmapTable->setItem(
                static_cast<int>(row),
                static_cast<int>(column),
                heatmapItem
            );
        }
    }

    distanceMatrixTable
        ->horizontalHeader()
        ->setSectionResizeMode(
            QHeaderView::Stretch
        );

    distanceMatrixTable
        ->verticalHeader()
        ->setSectionResizeMode(
            QHeaderView::Fixed
        );

    distanceHeatmapTable
        ->horizontalHeader()
        ->setSectionResizeMode(
            QHeaderView::Stretch
        );

    distanceHeatmapTable
        ->verticalHeader()
        ->setSectionResizeMode(
            QHeaderView::Fixed
        );
}

void MainWindow::generatePhylogeneticTree()
{
    if (loadedSequences.size() < 2)
    {
        QMessageBox::warning(
            this,
            "Insufficient Sequences",
            "Import at least two compatible sequences."
        );

        return;
    }

    auto aligner = createAligner(
        treeAlignmentMethodBox->currentText()
    );

    try
    {
        treeDistanceMatrix.calculate(
            loadedSequences,
            *aligner
        );

        currentTree.build(
            treeDistanceMatrix
        );

        int requiredTreeHeight = std::max(
            300,
            static_cast<int>(loadedSequences.size()) * 42
        );
        treeGraphic->setMinimumHeight(requiredTreeHeight);

        treeGraphic->setTree(
            &currentTree
        );

        treeOutput->setPlainText(
            QString::fromStdString(
                currentTree.toNewick()
            )
        );

        treeStatusLabel->setText(
            QString(
                "UPGMA tree generated using %1"
            ).arg(
                treeAlignmentMethodBox->currentText()
            )
        );

        treeStatusLabel->setStyleSheet(
            "font-weight: bold;"
            "color: #6EE7B7;"
        );

        recordHistory(
            "Phylogenetic Analysis",
            "Generate UPGMA Tree",
            "Completed",
            QString("Tree built from %1 sequence(s) using %2.")
                .arg(static_cast<qulonglong>(loadedSequences.size()))
                .arg(treeAlignmentMethodBox->currentText())
        );
    }
    catch (const std::exception& error)
    {
        treeDistanceMatrix.clear();
        currentTree.clear();

        treeGraphic->clearTree();
        treeOutput->clear();

        treeStatusLabel->setText(
            "Tree generation failed"
        );

        treeStatusLabel->setStyleSheet(
            "font-weight: bold;"
            "color: #FCA5A5;"
        );

        QMessageBox::critical(
            this,
            "UPGMA Tree Error",
            QString::fromStdString(
                error.what()
            )
        );
    }
}

void MainWindow::exportMatrixResults()
{
    if (currentDistanceMatrix.size() == 0)
    {
        QMessageBox::warning(
            this,
            "No Matrix",
            "Generate the distance matrix first."
        );

        return;
    }

    QString directory =
        QFileDialog::getExistingDirectory(
            this,
            "Select Matrix Export Folder"
        );

    if (directory.isEmpty())
    {
        return;
    }

    QDir resultsDirectory(directory);

    try
    {
        PhylogeneticExporter::exportCSV(
            currentDistanceMatrix,
            resultsDirectory.filePath(
                "distance_matrix.csv"
            ).toStdString()
        );

        PhylogeneticExporter::exportPhylip(
            currentDistanceMatrix,
            resultsDirectory.filePath(
                "distance_matrix.phy"
            ).toStdString()
        );

        QMessageBox::information(
            this,
            "Export Complete",
            "Exported:\n"
            "- distance_matrix.csv\n"
            "- distance_matrix.phy"
        );
    }
    catch (const std::exception& error)
    {
        QMessageBox::critical(
            this,
            "Export Error",
            QString::fromStdString(
                error.what()
            )
        );
    }
}

void MainWindow::exportTreeResults()
{
    if (currentTree.isEmpty())
    {
        QMessageBox::warning(
            this,
            "No Tree",
            "Generate the UPGMA tree first."
        );

        return;
    }

    QString directory =
        QFileDialog::getExistingDirectory(
            this,
            "Select Tree Export Folder"
        );

    if (directory.isEmpty())
    {
        return;
    }

    QDir resultsDirectory(directory);

    QString newickPath =
        resultsDirectory.filePath(
            "phylogenetic_tree.newick"
        );

    QString imagePath =
        resultsDirectory.filePath(
            "phylogenetic_tree.png"
        );

    try
    {
        PhylogeneticExporter::exportNewick(
            currentTree,
            newickPath.toStdString()
        );

        QPixmap treeImage =
            treeGraphic->grab();

        if (!treeImage.save(
                imagePath,
                "PNG"
            ))
        {
            throw std::runtime_error(
                "Could not export tree image."
            );
        }

        QMessageBox::information(
            this,
            "Export Complete",
            "Exported:\n"
            "- phylogenetic_tree.newick\n"
            "- phylogenetic_tree.png"
        );
    }
    catch (const std::exception& error)
    {
        QMessageBox::critical(
            this,
            "Export Error",
            QString::fromStdString(
                error.what()
            )
        );
    }
}

void MainWindow::importExpressionFile()
{
    QString filePath =
        QFileDialog::getOpenFileName(
            this,
            "Select Gene Expression File",
            QString(),
            "Expression Files (*.csv *.tsv);;"
            "All Files (*.*)"
        );

    if (filePath.isEmpty())
    {
        return;
    }

    loadExpressionFile(filePath, true);
}

void MainWindow::loadExpressionFile(
    const QString& filePath,
    bool shouldRecordHistory
)
{

    ExpressionParser parser;
    expressionQualityButton->setEnabled(false);

    try
    {
        ExpressionDataset parsedDataset =
            parser.parseFile(
                filePath.toStdString()
            );

        expressionDataset =
            std::make_unique<ExpressionDataset>(
                parsedDataset
            );

        sampleGrouping.automaticallyAssign(
            expressionDataset->getSampleNames()
        );
        expressionResults.clear();
        classifiedExpressionResults.clear();
        filteredExpressionResults.clear();
        currentNormalizedExpressionValues.clear();
        enrichmentResults.clear();
        filteredEnrichmentResults.clear();
        configureExpressionButton->setEnabled(true);
        expressionQualityButton->setEnabled(true);

        selectedExpressionFile = filePath;

        QFileInfo fileInformation(filePath);

        expressionFileLabel->setText(
            "Selected file: "
            + fileInformation.fileName()
        );

        expressionFileLabel->setToolTip(
            filePath
        );

        std::size_t geneCount =
            expressionDataset->getGeneCount();

        std::size_t sampleCount =
            expressionDataset->getSampleCount();

        const std::size_t maximumPreviewRows =
            200;

        std::size_t displayedGeneCount =
            std::min(
                geneCount,
                maximumPreviewRows
            );

        expressionPreviewTable->clear();

        expressionPreviewTable->setRowCount(
            static_cast<int>(
                displayedGeneCount
            )
        );

        expressionPreviewTable->setColumnCount(
            static_cast<int>(
                sampleCount
            )
        );

        QStringList sampleLabels;

        for (const std::string& sample :
             expressionDataset->getSampleNames())
        {
            sampleLabels.append(
                QString::fromStdString(sample)
            );
        }

        expressionPreviewTable
            ->setHorizontalHeaderLabels(
                sampleLabels
            );

        QStringList geneLabels;

        for (std::size_t gene = 0;
             gene < displayedGeneCount;
             ++gene)
        {
            geneLabels.append(
                QString::fromStdString(
                    expressionDataset
                        ->getGeneNames()
                        .at(gene)
                )
            );

            for (std::size_t sample = 0;
                 sample < sampleCount;
                 ++sample)
            {
                double value =
                    expressionDataset->getValue(
                        gene,
                        sample
                    );

                QTableWidgetItem* item =
                    new QTableWidgetItem(
                        QString::number(
                            value,
                            'f',
                            3
                        )
                    );

                item->setTextAlignment(
                    Qt::AlignCenter
                );

                expressionPreviewTable->setItem(
                    static_cast<int>(gene),
                    static_cast<int>(sample),
                    item
                );
            }
        }

        expressionPreviewTable
            ->setVerticalHeaderLabels(
                geneLabels
            );

        expressionPreviewTable
            ->horizontalHeader()
            ->setSectionResizeMode(
                QHeaderView::ResizeToContents
            );

        expressionPreviewTable
            ->verticalHeader()
            ->setSectionResizeMode(
                QHeaderView::ResizeToContents
            );

        QString summary =
            QString(
                "Valid dataset | %1 genes | "
                "%2 samples | Status: %3"
            )
            .arg(
                static_cast<qulonglong>(
                    geneCount
                )
            )
            .arg(
                static_cast<qulonglong>(
                    sampleCount
                )
            )
            .arg(
                QString::fromStdString(
                    expressionDataset
                        ->getStatusName()
                )
            );

        if (geneCount > maximumPreviewRows)
        {
            summary += QString(
                " | Previewing first %1 genes"
            ).arg(
                static_cast<qulonglong>(
                    maximumPreviewRows
                )
            );
        }

        expressionSummaryLabel->setText(
            summary
        );

        expressionSummaryLabel->setStyleSheet(
            "font-weight: bold;"
            "color: #6EE7B7;"
        );

        if (shouldRecordHistory)
        {
            recordHistory(
                "Gene Expression Analysis",
                "Import Expression Dataset",
                "Completed",
                QString("Loaded %1 genes across %2 samples.")
                    .arg(static_cast<qulonglong>(geneCount))
                    .arg(static_cast<qulonglong>(sampleCount))
            );
        }
    }
    catch (const std::exception& error)
    {
        expressionDataset.reset();
        sampleGrouping.clear();
        expressionResults.clear();
        classifiedExpressionResults.clear();
        filteredExpressionResults.clear();
        currentNormalizedExpressionValues.clear();
        enrichmentResults.clear();
        filteredEnrichmentResults.clear();
        configureExpressionButton->setEnabled(false);
        expressionQualityButton->setEnabled(false);

        expressionPreviewTable->clear();
        expressionPreviewTable->setRowCount(0);
        expressionPreviewTable->setColumnCount(0);

        expressionFileLabel->setText(
            "Expression file import failed"
        );

        expressionSummaryLabel->setText(
            "Invalid expression dataset"
        );

        expressionSummaryLabel->setStyleSheet(
            "font-weight: bold;"
            "color: #FCA5A5;"
        );

        QMessageBox::critical(
            this,
            "Expression Import Error",
            QString::fromStdString(
                error.what()
            )
        );
    }
}

void MainWindow::recordHistory(
    const QString& workspace,
    const QString& action,
    const QString& status,
    const QString& details
)
{
    projectSession.addHistory(workspace, action, status, details);
    markProjectModified();

    if (projectHistoryTable != nullptr)
    {
        refreshProjectHistoryTable();
    }
}

void MainWindow::markProjectModified()
{
    projectModified = true;
    updateWindowTitle();
}

void MainWindow::updateWindowTitle()
{
    QString title = "BioFlow Studio";

    if (!projectSession.projectName.isEmpty()
        && projectSession.projectName != "Untitled BioFlow Project")
    {
        title += " - " + projectSession.projectName;
    }

    if (projectModified)
    {
        title += " *";
    }

    setWindowTitle(title);
}

bool MainWindow::confirmDiscardUnsavedChanges()
{
    if (!projectModified)
    {
        return true;
    }

    QMessageBox messageBox(this);
    messageBox.setWindowTitle("Unsaved Project Changes");
    messageBox.setIcon(QMessageBox::Warning);
    messageBox.setText("The current BioFlow project has unsaved changes.");
    messageBox.setInformativeText(
        "Would you like to save the project before continuing?"
    );
    messageBox.setStandardButtons(
        QMessageBox::Save
        | QMessageBox::Discard
        | QMessageBox::Cancel
    );
    messageBox.setDefaultButton(QMessageBox::Save);

    int choice = messageBox.exec();

    if (choice == QMessageBox::Save)
    {
        saveProject();
        return !projectModified;
    }

    return choice == QMessageBox::Discard;
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    if (confirmDiscardUnsavedChanges())
    {
        event->accept();
    }
    else
    {
        event->ignore();
    }
}

void MainWindow::showAboutDialog()
{
    QMessageBox::about(
        this,
        "About BioFlow Studio",
        "<h2 style='color:#F8FAFC;'>BioFlow Studio 1.0</h2>"
        "<p><b>Integrated biological analysis and workflow platform</b></p>"
        "<p>BioFlow Studio combines phylogenetic analysis, gene-expression "
        "analysis, quality control, pathway enrichment, interactive "
        "visualization and reproducible project workflows.</p>"
        "<p><b>Technology:</b> C++17, Qt 6, CMake and MinGW</p>"
        "<p><b>OOP design:</b> abstraction, inheritance, polymorphism, "
        "encapsulation and strategy-based algorithms</p>"
        "<p><b>Project leader and integrator:</b> Muhammad Abdul Wahid<br>"
        "Developed as a four-member undergraduate bioinformatics project.</p>"
        "<p style='color:#94A3B8;'>Educational software &mdash; analytical "
        "results should be validated before research or clinical use.</p>"
    );
}

void MainWindow::refreshProjectHistoryTable()
{
    if (projectHistoryTable == nullptr)
    {
        return;
    }

    const auto& history = projectSession.history;
    projectHistoryTable->setSortingEnabled(false);
    projectHistoryTable->clearContents();
    projectHistoryTable->setRowCount(static_cast<int>(history.size()));

    for (std::size_t displayRow = 0;
         displayRow < history.size();
         ++displayRow)
    {
        const AnalysisHistoryEntry& entry =
            history.at(history.size() - displayRow - 1);
        int row = static_cast<int>(displayRow);

        projectHistoryTable->setItem(
            row, 0, new QTableWidgetItem(entry.timestamp)
        );
        projectHistoryTable->setItem(
            row, 1, new QTableWidgetItem(entry.workspace)
        );
        projectHistoryTable->setItem(
            row, 2, new QTableWidgetItem(entry.action)
        );

        QTableWidgetItem* statusItem = new QTableWidgetItem(entry.status);
        statusItem->setTextAlignment(Qt::AlignCenter);

        if (entry.status.compare("Completed", Qt::CaseInsensitive) == 0)
        {
            statusItem->setBackground(QColor("#CDEFD8"));
            statusItem->setForeground(QColor("#176B35"));
        }
        else if (entry.status.compare("Failed", Qt::CaseInsensitive) == 0)
        {
            statusItem->setBackground(QColor("#F8D7DA"));
            statusItem->setForeground(QColor("#9C1C1C"));
        }
        else
        {
            statusItem->setBackground(QColor("#FFF1C7"));
            statusItem->setForeground(QColor("#8A5A00"));
        }

        projectHistoryTable->setItem(row, 3, statusItem);
        projectHistoryTable->setItem(
            row, 4, new QTableWidgetItem(entry.details)
        );
    }

    projectHistorySummaryLabel->setText(
        history.empty()
            ? "No history entries yet."
            : QString("%1 recorded project event(s) | Newest first")
                .arg(static_cast<qulonglong>(history.size()))
    );
}

void MainWindow::saveProject()
{
    QString filePath = currentProjectFile;

    if (filePath.isEmpty())
    {
        filePath = QFileDialog::getSaveFileName(
            this,
            "Save BioFlow Project",
            "BioFlow_Project.bioflow",
            "BioFlow Projects (*.bioflow)"
        );
    }

    if (filePath.isEmpty())
    {
        return;
    }

    if (!filePath.endsWith(".bioflow", Qt::CaseInsensitive))
    {
        filePath += ".bioflow";
    }

    projectSession.projectName = QFileInfo(filePath).completeBaseName();
    projectSession.savedAt = QDateTime::currentDateTime().toString(Qt::ISODate);
    projectSession.fastaFilePaths = selectedFastaFiles;
    projectSession.expressionFilePath = selectedExpressionFile;
    projectSession.matrixMethodIndex = matrixAlignmentMethodBox->currentIndex();
    projectSession.treeMethodIndex = treeAlignmentMethodBox->currentIndex();
    projectSession.normalizationMethodIndex =
        normalizationMethodBox->currentIndex();
    projectSession.adjustedPValueThreshold = adjustedPThresholdBox->value();
    projectSession.foldChangeThreshold = foldChangeThresholdBox->value();
    projectSession.enrichmentPValueThreshold =
        enrichmentPThresholdBox->value();
    projectSession.workflowTemplateIndex = workflowTemplateBox->currentIndex();
    projectSession.sampleGroups.clear();

    if (!sampleGroupBoxes.empty())
    {
        for (const QComboBox* groupBox : sampleGroupBoxes)
        {
            projectSession.sampleGroups.push_back(groupBox->currentIndex());
        }
    }
    else
    {
        for (SampleGroup group : sampleGrouping.getGroups())
        {
            projectSession.sampleGroups.push_back(static_cast<int>(group));
        }
    }

    try
    {
        projectSession.addHistory(
            "Project",
            "Save Project",
            "Completed",
            "Project saved to " + filePath
        );
        ProjectSerializer::save(filePath, projectSession);

        currentProjectFile = filePath;
        projectModified = false;
        updateWindowTitle();
        refreshProjectHistoryTable();
        statusLabel->setText(
            "Project saved: " + QFileInfo(filePath).fileName()
        );
        statusBar()->showMessage(
            "Project saved successfully: " + filePath,
            7000
        );

        QMessageBox::information(
            this,
            "Project Saved",
            "The project, analysis settings and history were saved.\n\n"
            "Project file: " + filePath
        );
    }
    catch (const std::exception& error)
    {
        if (!projectSession.history.empty()
            && projectSession.history.back().action == "Save Project"
            && projectSession.history.back().status == "Completed")
        {
            projectSession.history.pop_back();
        }

        recordHistory(
            "Project",
            "Save Project",
            "Failed",
            QString::fromStdString(error.what())
        );
        QMessageBox::critical(
            this,
            "Project Save Error",
            QString::fromStdString(error.what())
        );
    }
}

void MainWindow::loadProject()
{
    if (!confirmDiscardUnsavedChanges())
    {
        return;
    }

    QString filePath = QFileDialog::getOpenFileName(
        this,
        "Open BioFlow Project",
        QString(),
        "BioFlow Projects (*.bioflow);;JSON Files (*.json);;All Files (*.*)"
    );

    if (filePath.isEmpty())
    {
        return;
    }

    try
    {
        ProjectSession loadedSession = ProjectSerializer::load(filePath);
        projectSession = loadedSession;
        currentProjectFile = filePath;

        QStringList existingFastaFiles;
        QStringList missingFiles;

        for (const QString& fastaPath : projectSession.fastaFilePaths)
        {
            if (QFileInfo::exists(fastaPath))
            {
                existingFastaFiles.append(fastaPath);
            }
            else
            {
                missingFiles.append(fastaPath);
            }
        }

        if (!existingFastaFiles.isEmpty())
        {
            loadFastaFiles(existingFastaFiles, false);
        }

        if (!projectSession.expressionFilePath.isEmpty())
        {
            if (QFileInfo::exists(projectSession.expressionFilePath))
            {
                loadExpressionFile(projectSession.expressionFilePath, false);
            }
            else
            {
                missingFiles.append(projectSession.expressionFilePath);
            }
        }

        auto restoreComboIndex = [](QComboBox* box, int index)
        {
            if (box != nullptr && index >= 0 && index < box->count())
            {
                box->setCurrentIndex(index);
            }
        };

        restoreComboIndex(
            matrixAlignmentMethodBox,
            projectSession.matrixMethodIndex
        );
        restoreComboIndex(
            treeAlignmentMethodBox,
            projectSession.treeMethodIndex
        );
        restoreComboIndex(
            normalizationMethodBox,
            projectSession.normalizationMethodIndex
        );
        restoreComboIndex(
            workflowTemplateBox,
            projectSession.workflowTemplateIndex
        );

        adjustedPThresholdBox->setValue(
            projectSession.adjustedPValueThreshold
        );
        foldChangeThresholdBox->setValue(
            projectSession.foldChangeThreshold
        );
        enrichmentPThresholdBox->setValue(
            projectSession.enrichmentPValueThreshold
        );

        if (expressionDataset
            && projectSession.sampleGroups.size()
                == expressionDataset->getSampleCount())
        {
            for (std::size_t index = 0;
                 index < projectSession.sampleGroups.size();
                 ++index)
            {
                int groupValue = projectSession.sampleGroups.at(index);
                groupValue = std::clamp(groupValue, 0, 2);
                sampleGrouping.setGroup(
                    index,
                    static_cast<SampleGroup>(groupValue)
                );
            }

            populateSampleGroupingTable();
        }

        loadWorkflowTemplate();
        recordHistory(
            "Project",
            "Open Project",
            "Completed",
            "Loaded " + QFileInfo(filePath).fileName()
        );

        projectModified = false;
        updateWindowTitle();
        statusLabel->setText(
            "Project loaded: " + projectSession.projectName
        );
        statusBar()->showMessage(
            "Project loaded successfully: " + filePath,
            7000
        );
        pages->setCurrentIndex(DashboardPage);

        QString message =
            "Project data references, settings and analysis history were "
            "restored. Re-run analyses to regenerate calculated results.";

        if (!missingFiles.isEmpty())
        {
            message += "\n\nThe following source file(s) could not be found:\n- "
                + missingFiles.join("\n- ");
        }

        QMessageBox::information(
            this,
            "Project Loaded",
            message
        );
    }
    catch (const std::exception& error)
    {
        QMessageBox::critical(
            this,
            "Project Load Error",
            QString::fromStdString(error.what())
        );
    }
}

void MainWindow::populateQualityReportTable(
    QTableWidget* table,
    QLabel* summaryLabel,
    const QualityReport& report
)
{
    table->clearContents();
    table->setRowCount(
        static_cast<int>(report.getChecks().size())
    );

    for (std::size_t row = 0;
         row < report.getChecks().size();
         ++row)
    {
        const QualityCheck& check = report.getChecks().at(row);

        QTableWidgetItem* nameItem = new QTableWidgetItem(
            QString::fromStdString(check.name)
        );
        QTableWidgetItem* statusItem = new QTableWidgetItem(
            QString::fromStdString(
                QualityReport::statusName(check.status)
            )
        );
        QTableWidgetItem* observationItem = new QTableWidgetItem(
            QString::fromStdString(check.observation)
        );
        QTableWidgetItem* recommendationItem = new QTableWidgetItem(
            QString::fromStdString(check.recommendation)
        );

        statusItem->setTextAlignment(Qt::AlignCenter);

        QColor background;
        QColor foreground;

        if (check.status == QualityStatus::Pass)
        {
            background = QColor("#CDEFD8");
            foreground = QColor("#176B35");
        }
        else if (check.status == QualityStatus::Warning)
        {
            background = QColor("#FFF0BF");
            foreground = QColor("#8A5A00");
        }
        else
        {
            background = QColor("#FFD6D6");
            foreground = QColor("#9C1C1C");
        }

        statusItem->setBackground(background);
        statusItem->setForeground(foreground);

        int tableRow = static_cast<int>(row);
        table->setItem(tableRow, 0, nameItem);
        table->setItem(tableRow, 1, statusItem);
        table->setItem(tableRow, 2, observationItem);
        table->setItem(tableRow, 3, recommendationItem);
    }

    table->resizeRowsToContents();

    summaryLabel->setText(
        QString(
            "Overall status: %1 | %2 passed | %3 warning(s) | %4 failed"
        )
        .arg(QString::fromStdString(report.getOverallStatusName()))
        .arg(static_cast<qulonglong>(report.getPassCount()))
        .arg(static_cast<qulonglong>(report.getWarningCount()))
        .arg(static_cast<qulonglong>(report.getFailCount()))
    );

    if (report.getOverallStatus() == QualityStatus::Pass)
    {
        summaryLabel->setStyleSheet(
            "font-weight: 700; color: #6EE7B7;"
        );
    }
    else if (report.getOverallStatus() == QualityStatus::Warning)
    {
        summaryLabel->setStyleSheet(
            "font-weight: 700; color: #FBBF24;"
        );
    }
    else
    {
        summaryLabel->setStyleSheet(
            "font-weight: 700; color: #FCA5A5;"
        );
    }
}

void MainWindow::populateSampleGroupingTable()
{
    if (!expressionDataset)
    {
        return;
    }

    const auto& sampleNames = expressionDataset->getSampleNames();

    if (sampleGrouping.getGroups().size() != sampleNames.size())
    {
        sampleGrouping.automaticallyAssign(sampleNames);
    }

    sampleGroupingTable->clearContents();
    sampleGroupingTable->setRowCount(
        static_cast<int>(sampleNames.size())
    );
    sampleGroupBoxes.clear();
    sampleGroupBoxes.reserve(sampleNames.size());

    for (std::size_t index = 0; index < sampleNames.size(); ++index)
    {
        QTableWidgetItem* sampleItem = new QTableWidgetItem(
            QString::fromStdString(sampleNames.at(index))
        );
        sampleItem->setTextAlignment(Qt::AlignCenter);

        QComboBox* groupBox = new QComboBox;
        groupBox->addItems(
            {"Unassigned", "Control", "Treatment"}
        );

        SampleGroup group = sampleGrouping.getGroup(index);

        if (group == SampleGroup::Control)
        {
            groupBox->setCurrentIndex(1);
        }
        else if (group == SampleGroup::Treatment)
        {
            groupBox->setCurrentIndex(2);
        }
        else
        {
            groupBox->setCurrentIndex(0);
        }

        sampleGroupingTable->setItem(
            static_cast<int>(index),
            0,
            sampleItem
        );
        sampleGroupingTable->setCellWidget(
            static_cast<int>(index),
            1,
            groupBox
        );

        sampleGroupBoxes.push_back(groupBox);

        connect(
            groupBox,
            &QComboBox::currentIndexChanged,
            this,
            [this]()
            {
                markProjectModified();

                std::size_t controlCount = 0;
                std::size_t treatmentCount = 0;

                for (const QComboBox* box : sampleGroupBoxes)
                {
                    if (box->currentIndex() == 1)
                    {
                        ++controlCount;
                    }
                    else if (box->currentIndex() == 2)
                    {
                        ++treatmentCount;
                    }
                }

                groupingStatusLabel->setText(
                    QString("Control: %1 sample(s) | Treatment: %2 sample(s)")
                        .arg(static_cast<qulonglong>(controlCount))
                        .arg(static_cast<qulonglong>(treatmentCount))
                );

                bool valid = controlCount >= 2 && treatmentCount >= 2;
                groupingStatusLabel->setStyleSheet(
                    valid
                        ? "font-weight: 700; color: #6EE7B7;"
                        : "font-weight: 700; color: #FBBF24;"
                );
            }
        );
    }

    std::size_t controlCount = 0;
    std::size_t treatmentCount = 0;

    for (const QComboBox* box : sampleGroupBoxes)
    {
        controlCount += box->currentIndex() == 1 ? 1 : 0;
        treatmentCount += box->currentIndex() == 2 ? 1 : 0;
    }

    groupingStatusLabel->setText(
        QString("Control: %1 sample(s) | Treatment: %2 sample(s)")
            .arg(static_cast<qulonglong>(controlCount))
            .arg(static_cast<qulonglong>(treatmentCount))
    );

    groupingStatusLabel->setStyleSheet(
        controlCount >= 2 && treatmentCount >= 2
            ? "font-weight: 700; color: #6EE7B7;"
            : "font-weight: 700; color: #FBBF24;"
    );
}

void MainWindow::runDifferentialExpressionAnalysis()
{
    if (!expressionDataset)
    {
        QMessageBox::warning(
            this,
            "No Expression Dataset",
            "Please import a valid expression dataset first."
        );
        return;
    }

    sampleGrouping.automaticallyAssign(
        expressionDataset->getSampleNames()
    );

    for (std::size_t index = 0;
         index < sampleGroupBoxes.size();
         ++index)
    {
        SampleGroup group = SampleGroup::Unassigned;

        if (sampleGroupBoxes.at(index)->currentIndex() == 1)
        {
            group = SampleGroup::Control;
        }
        else if (sampleGroupBoxes.at(index)->currentIndex() == 2)
        {
            group = SampleGroup::Treatment;
        }

        sampleGrouping.setGroup(index, group);
    }

    if (!sampleGrouping.isValid())
    {
        QMessageBox::warning(
            this,
            "Invalid Sample Groups",
            "Assign at least two samples to Control and at least two "
            "samples to Treatment."
        );
        return;
    }

    try
    {
        std::unique_ptr<NormalizationStrategy> normalizer;

        if (normalizationMethodBox->currentIndex() == 0)
        {
            normalizer = std::make_unique<RawNormalization>();
        }
        else if (normalizationMethodBox->currentIndex() == 1)
        {
            normalizer = std::make_unique<Log2Normalization>();
        }
        else
        {
            normalizer = std::make_unique<ZScoreNormalization>();
        }

        currentNormalizedExpressionValues =
            normalizer->normalize(*expressionDataset);

        WelchTTestAnalyzer analyzer;
        expressionResults = analyzer.analyze(
            *expressionDataset,
            currentNormalizedExpressionValues,
            sampleGrouping
        );

        enrichmentResults.clear();
        filteredEnrichmentResults.clear();

        resetExpressionFilters();
        pages->setCurrentIndex(ExpressionResultsPage);

        recordHistory(
            "Gene Expression Analysis",
            "Differential Expression",
            "Completed",
            QString("Analyzed %1 gene(s) using %2 normalization.")
                .arg(static_cast<qulonglong>(expressionResults.size()))
                .arg(normalizationMethodBox->currentText())
        );
    }
    catch (const std::exception& error)
    {
        QMessageBox::critical(
            this,
            "Differential Expression Error",
            QString::fromStdString(error.what())
        );
    }
}

ExpressionFilterSettings
MainWindow::getCurrentExpressionFilterSettings() const
{
    ExpressionFilterSettings settings;

    settings.geneQuery = geneSearchBox->text().toStdString();
    settings.adjustedPValueThreshold = adjustedPThresholdBox->value();
    settings.minimumAbsoluteLog2FoldChange =
        foldChangeThresholdBox->value();
    settings.maximumResults = static_cast<std::size_t>(
        maximumResultsBox->currentData().toInt()
    );

    switch (regulationFilterBox->currentIndex())
    {
        case 1:
            settings.regulationFilter = RegulationFilter::Upregulated;
            break;

        case 2:
            settings.regulationFilter = RegulationFilter::Downregulated;
            break;

        case 3:
            settings.regulationFilter = RegulationFilter::NotSignificant;
            break;

        default:
            settings.regulationFilter = RegulationFilter::All;
            break;
    }

    return settings;
}

void MainWindow::applyExpressionFilters()
{
    if (expressionResults.empty())
    {
        return;
    }

    ExpressionFilterSettings settings =
        getCurrentExpressionFilterSettings();

    ExpressionFilterEngine filterEngine;
    classifiedExpressionResults = filterEngine.classify(
        expressionResults,
        settings
    );
    filteredExpressionResults = filterEngine.filter(
        classifiedExpressionResults,
        settings
    );

    populateExpressionResultsTable();

    expressionFilterSummaryLabel->setText(
        QString(
            "Showing %1 of %2 genes | Adjusted p <= %3 | "
            "Minimum |log2 FC| = %4"
        )
        .arg(static_cast<qulonglong>(filteredExpressionResults.size()))
        .arg(static_cast<qulonglong>(classifiedExpressionResults.size()))
        .arg(settings.adjustedPValueThreshold, 0, 'g', 5)
        .arg(settings.minimumAbsoluteLog2FoldChange, 0, 'f', 2)
    );

    expressionFilterSummaryLabel->setStyleSheet(
        filteredExpressionResults.empty()
            ? "font-weight: 700; color: #FBBF24;"
            : "font-weight: 700; color: #6EE7B7;"
    );
}

void MainWindow::resetExpressionFilters()
{
    geneSearchBox->clear();
    regulationFilterBox->setCurrentIndex(0);
    adjustedPThresholdBox->setValue(0.05);
    foldChangeThresholdBox->setValue(1.0);
    maximumResultsBox->setCurrentIndex(0);

    applyExpressionFilters();
}

void MainWindow::populateExpressionResultsTable()
{
    expressionResultsTable->setSortingEnabled(false);
    expressionResultsTable->clearContents();
    expressionResultsTable->setRowCount(
        static_cast<int>(filteredExpressionResults.size())
    );

    std::size_t upregulatedCount = 0;
    std::size_t downregulatedCount = 0;

    for (const DifferentialExpressionResult& result :
         classifiedExpressionResults)
    {
        if (result.getRegulationStatus() == RegulationStatus::Upregulated)
        {
            ++upregulatedCount;
        }
        else if (result.getRegulationStatus()
                 == RegulationStatus::Downregulated)
        {
            ++downregulatedCount;
        }
    }

    for (std::size_t row = 0;
         row < filteredExpressionResults.size();
         ++row)
    {
        const DifferentialExpressionResult& result =
            filteredExpressionResults.at(row);

        QTableWidgetItem* geneItem = new QTableWidgetItem(
            QString::fromStdString(result.getGeneName())
        );
        geneItem->setTextAlignment(Qt::AlignCenter);

        QTableWidgetItem* regulationItem = new QTableWidgetItem(
            QString::fromStdString(result.getRegulationName())
        );
        regulationItem->setTextAlignment(Qt::AlignCenter);

        if (result.getRegulationStatus() == RegulationStatus::Upregulated)
        {
            regulationItem->setBackground(QColor("#CDEFD8"));
            regulationItem->setForeground(QColor("#176B35"));
        }
        else if (result.getRegulationStatus() == RegulationStatus::Downregulated)
        {
            regulationItem->setBackground(QColor("#FFD6D6"));
            regulationItem->setForeground(QColor("#9C1C1C"));
        }
        else
        {
            regulationItem->setBackground(QColor("#E8EDF2"));
            regulationItem->setForeground(QColor("#40566B"));
        }

        int tableRow = static_cast<int>(row);
        expressionResultsTable->setItem(tableRow, 0, geneItem);
        expressionResultsTable->setItem(
            tableRow, 1,
            new NumericTableWidgetItem(result.getControlMean(), 7)
        );
        expressionResultsTable->setItem(
            tableRow, 2,
            new NumericTableWidgetItem(result.getTreatmentMean(), 7)
        );
        expressionResultsTable->setItem(
            tableRow, 3,
            new NumericTableWidgetItem(result.getLog2FoldChange(), 7)
        );
        expressionResultsTable->setItem(
            tableRow, 4,
            new NumericTableWidgetItem(result.getPValue(), 7)
        );
        expressionResultsTable->setItem(
            tableRow, 5,
            new NumericTableWidgetItem(result.getAdjustedPValue(), 7)
        );
        expressionResultsTable->setItem(tableRow, 6, regulationItem);
    }

    std::size_t significantCount =
        upregulatedCount + downregulatedCount;

    expressionResultsSummaryLabel->setText(
        QString(
            "%1 genes analyzed | %2 significant | "
            "%3 upregulated | %4 downregulated | Normalization: %5"
        )
        .arg(static_cast<qulonglong>(classifiedExpressionResults.size()))
        .arg(static_cast<qulonglong>(significantCount))
        .arg(static_cast<qulonglong>(upregulatedCount))
        .arg(static_cast<qulonglong>(downregulatedCount))
        .arg(normalizationMethodBox->currentText())
    );
    expressionResultsSummaryLabel->setStyleSheet(
        "font-weight: 700; color: #6EE7B7;"
    );

    expressionResultsTable->setSortingEnabled(true);
    expressionResultsTable->sortItems(5, Qt::AscendingOrder);
}

void MainWindow::exportExpressionTables()
{
    if (!expressionDataset
        || filteredExpressionResults.empty()
        || currentNormalizedExpressionValues.empty())
    {
        QMessageBox::warning(
            this,
            "Nothing to Export",
            "Run differential expression analysis before exporting results."
        );
        return;
    }

    QString directoryPath = QFileDialog::getExistingDirectory(
        this,
        "Select Analysis Export Folder",
        QString(),
        QFileDialog::ShowDirsOnly
    );

    if (directoryPath.isEmpty())
    {
        return;
    }

    try
    {
        QDir exportDirectory(directoryPath);
        GeneExpressionExporter exporter;

        exporter.exportResultsCSV(
            exportDirectory
                .filePath("differential_expression_results.csv")
                .toStdString(),
            filteredExpressionResults
        );

        exporter.exportNormalizedMatrixCSV(
            exportDirectory
                .filePath("normalized_expression_matrix.csv")
                .toStdString(),
            *expressionDataset,
            currentNormalizedExpressionValues,
            sampleGrouping
        );

        exporter.exportAnalysisSummary(
            exportDirectory
                .filePath("analysis_summary.txt")
                .toStdString(),
            *expressionDataset,
            classifiedExpressionResults,
            sampleGrouping,
            normalizationMethodBox->currentText().toStdString(),
            adjustedPThresholdBox->value(),
            foldChangeThresholdBox->value()
        );

        QMessageBox::information(
            this,
            "Export Completed",
            "Three files were exported successfully:\n\n"
            "differential_expression_results.csv\n"
            "normalized_expression_matrix.csv\n"
            "analysis_summary.txt\n\n"
            "Folder: " + directoryPath
        );
    }
    catch (const std::exception& error)
    {
        QMessageBox::critical(
            this,
            "Expression Export Error",
            QString::fromStdString(error.what())
        );
    }
}

void MainWindow::exportEnrichmentResults()
{
    if (filteredEnrichmentResults.empty())
    {
        QMessageBox::warning(
            this,
            "Nothing to Export",
            "No enrichment results match the current filters."
        );
        return;
    }

    QString filePath = QFileDialog::getSaveFileName(
        this,
        "Export Functional Enrichment Results",
        "functional_enrichment_results.csv",
        "CSV Files (*.csv)"
    );

    if (filePath.isEmpty())
    {
        return;
    }

    if (!filePath.endsWith(".csv", Qt::CaseInsensitive))
    {
        filePath += ".csv";
    }

    try
    {
        std::ofstream output(filePath.toStdString());

        if (!output.is_open())
        {
            throw std::runtime_error(
                "Could not create the enrichment CSV file."
            );
        }

        auto quoteCSV = [](const std::string& value)
        {
            std::string escaped;
            escaped.reserve(value.size() + 2);
            escaped.push_back('"');

            for (char character : value)
            {
                if (character == '"')
                {
                    escaped.push_back('"');
                }

                escaped.push_back(character);
            }

            escaped.push_back('"');
            return escaped;
        };

        output
            << "Pathway ID,Pathway,Category,Overlap Count,"
            << "Pathway Genes in Background,Fold Enrichment,P-value,"
            << "Adjusted P-value,Contributing Genes\n";

        output << std::setprecision(10);

        for (const EnrichmentResult& result : filteredEnrichmentResults)
        {
            output
                << quoteCSV(result.getPathwayId()) << ','
                << quoteCSV(result.getPathwayName()) << ','
                << quoteCSV(result.getCategory()) << ','
                << result.getOverlapCount() << ','
                << result.getPathwayGeneCount() << ','
                << result.getFoldEnrichment() << ','
                << result.getPValue() << ','
                << result.getAdjustedPValue() << ','
                << quoteCSV(result.getOverlappingGeneList())
                << '\n';
        }

        QMessageBox::information(
            this,
            "Export Completed",
            "Functional-enrichment results were exported successfully.\n\n"
            "File: " + filePath
        );
    }
    catch (const std::exception& error)
    {
        QMessageBox::critical(
            this,
            "Enrichment Export Error",
            QString::fromStdString(error.what())
        );
    }
}

bool MainWindow::saveReportImage(
    QWidget* widget,
    const QString& filePath,
    int width,
    int height
)
{
    if (widget == nullptr)
    {
        return false;
    }

    QSize originalSize = widget->size();
    widget->resize(width, height);
    widget->ensurePolished();

    QChartView* chartView = qobject_cast<QChartView*>(widget);

    if (chartView != nullptr && chartView->chart() != nullptr)
    {
        chartView->chart()->setAnimationOptions(QChart::NoAnimation);
        chartView->chart()->resize(width, height);
        chartView->chart()->update();
        chartView->viewport()->update();
    }

    QApplication::processEvents();

    QPixmap image(width, height);
    image.fill(Qt::white);

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    widget->render(&painter);
    painter.end();

    bool saved = image.save(filePath, "PNG");

    widget->resize(originalSize);
    return saved;
}

void MainWindow::generateExpressionHtmlReport()
{
    if (!expressionDataset
        || classifiedExpressionResults.empty()
        || currentNormalizedExpressionValues.empty())
    {
        QMessageBox::warning(
            this,
            "No Expression Analysis",
            "Run differential-expression analysis before generating a report."
        );
        return;
    }

    QString parentPath = QFileDialog::getExistingDirectory(
        this,
        "Select Folder for Expression Analysis Report",
        QString(),
        QFileDialog::ShowDirsOnly
    );

    if (parentPath.isEmpty())
    {
        return;
    }

    try
    {
        QString timeStamp = QDateTime::currentDateTime().toString(
            "yyyyMMdd_HHmmss"
        );
        QString folderName = "BioFlow_Expression_Report_" + timeStamp;
        QDir parentDirectory(parentPath);

        if (!parentDirectory.mkpath(folderName))
        {
            throw std::runtime_error("Could not create the report folder.");
        }

        QDir reportDirectory(parentDirectory.filePath(folderName));
        AnalysisReport report;
        report.title = "BioFlow Studio Gene-Expression Analysis Report";
        report.subtitle = "Differential expression, quality control, "
            "visualization and pathway interpretation";
        report.generatedAt = QDateTime::currentDateTime()
            .toString("dddd, dd MMMM yyyy — hh:mm AP")
            .toStdString();
        report.overview =
            "This automatically generated report summarizes the imported "
            "expression matrix, statistical analysis, significant genes, "
            "quality checks and graphical results.";

        std::size_t significantCount = 0;
        std::size_t upregulatedCount = 0;
        std::size_t downregulatedCount = 0;

        for (const DifferentialExpressionResult& result :
             classifiedExpressionResults)
        {
            if (result.getRegulationStatus()
                == RegulationStatus::Upregulated)
            {
                ++significantCount;
                ++upregulatedCount;
            }
            else if (result.getRegulationStatus()
                     == RegulationStatus::Downregulated)
            {
                ++significantCount;
                ++downregulatedCount;
            }
        }

        report.metadata = {
            {"Dataset", QFileInfo(selectedExpressionFile)
                .fileName().toStdString()},
            {"Genes", std::to_string(expressionDataset->getGeneCount())},
            {"Samples", std::to_string(expressionDataset->getSampleCount())},
            {"Normalization", normalizationMethodBox
                ->currentText().toStdString()},
            {"Statistical Test", "Welch two-sample t-test"},
            {"Multiple Testing", "Benjamini-Hochberg FDR correction"},
            {"Significant Genes", std::to_string(significantCount)},
            {"Up / Down", std::to_string(upregulatedCount)
                + " / " + std::to_string(downregulatedCount)},
            {"Control / Treatment", std::to_string(
                sampleGrouping.getControlCount()) + " / "
                + std::to_string(sampleGrouping.getTreatmentCount())}
        };

        ExpressionQualityAnalyzer qualityAnalyzer(
            *expressionDataset,
            sampleGrouping
        );
        QualityReport qualityReport = qualityAnalyzer.analyze();
        ReportTable qualityTable;
        qualityTable.title = "Expression Data-Quality Assessment";
        qualityTable.headers = {
            "Check", "Status", "Observation", "Recommendation"
        };

        for (const QualityCheck& check : qualityReport.getChecks())
        {
            qualityTable.rows.push_back({
                check.name,
                QualityReport::statusName(check.status),
                check.observation,
                check.recommendation
            });
        }

        report.tables.push_back(qualityTable);

        ReportTable resultTable;
        resultTable.title = "Differential-Expression Results";
        resultTable.headers = {
            "Gene", "Control Mean", "Treatment Mean", "Log2 Fold Change",
            "P-value", "Adjusted P-value", "Regulation"
        };

        std::size_t resultLimit = std::min<std::size_t>(
            100,
            classifiedExpressionResults.size()
        );

        for (std::size_t index = 0; index < resultLimit; ++index)
        {
            const DifferentialExpressionResult& result =
                classifiedExpressionResults.at(index);

            resultTable.rows.push_back({
                result.getGeneName(),
                QString::number(result.getControlMean(), 'g', 7).toStdString(),
                QString::number(result.getTreatmentMean(), 'g', 7).toStdString(),
                QString::number(result.getLog2FoldChange(), 'g', 7).toStdString(),
                QString::number(result.getPValue(), 'g', 7).toStdString(),
                QString::number(result.getAdjustedPValue(), 'g', 7).toStdString(),
                result.getRegulationName()
            });
        }

        report.tables.push_back(resultTable);

        if (!enrichmentResults.empty())
        {
            ReportTable pathwayTable;
            pathwayTable.title = "Functional Enrichment Results";
            pathwayTable.headers = {
                "Pathway", "Category", "Overlap", "Fold Enrichment",
                "Adjusted P-value", "Contributing Genes"
            };

            std::size_t pathwayLimit = std::min<std::size_t>(
                30,
                enrichmentResults.size()
            );

            for (std::size_t index = 0; index < pathwayLimit; ++index)
            {
                const EnrichmentResult& result = enrichmentResults.at(index);
                pathwayTable.rows.push_back({
                    result.getPathwayName(),
                    result.getCategory(),
                    std::to_string(result.getOverlapCount()) + " / "
                        + std::to_string(result.getPathwayGeneCount()),
                    QString::number(
                        result.getFoldEnrichment(), 'g', 6
                    ).toStdString(),
                    QString::number(
                        result.getAdjustedPValue(), 'g', 7
                    ).toStdString(),
                    result.getOverlappingGeneList()
                });
            }

            report.tables.push_back(pathwayTable);
        }

        volcanoPlotWidget->setResults(
            classifiedExpressionResults,
            adjustedPThresholdBox->value(),
            foldChangeThresholdBox->value()
        );

        QString volcanoPath = reportDirectory.filePath("volcano_plot.png");

        if (saveReportImage(volcanoPlotWidget, volcanoPath))
        {
            report.images.push_back({
                "Volcano Plot", "volcano_plot.png",
                "Genes are positioned by log2 fold change and statistical "
                "significance."
            });
        }

        const auto& heatmapGenes = filteredExpressionResults.empty()
            ? classifiedExpressionResults
            : filteredExpressionResults;

        expressionHeatmapWidget->setData(
            *expressionDataset,
            currentNormalizedExpressionValues,
            heatmapGenes,
            sampleGrouping,
            50
        );

        QString heatmapPath = reportDirectory.filePath(
            "expression_heatmap.png"
        );

        if (saveReportImage(expressionHeatmapWidget, heatmapPath, 1200, 760))
        {
            report.images.push_back({
                "Gene-Expression Heatmap", "expression_heatmap.png",
                "Row Z-scores show relative expression patterns across samples."
            });
        }

        PCAAnalyzer pcaAnalyzer;
        PCAResult pcaResult = pcaAnalyzer.analyze(
            *expressionDataset,
            currentNormalizedExpressionValues
        );
        pcaPlotWidget->setResult(pcaResult, sampleGrouping);

        QString pcaPath = reportDirectory.filePath("pca_sample_plot.png");

        if (saveReportImage(pcaPlotWidget, pcaPath))
        {
            report.images.push_back({
                "PCA Sample Plot", "pca_sample_plot.png",
                "Nearby samples have similar overall expression profiles."
            });
        }

        if (!enrichmentResults.empty())
        {
            enrichmentBarChart->setResults(enrichmentResults, 10);
            QString pathwayPath = reportDirectory.filePath(
                "pathway_enrichment_chart.png"
            );

            if (saveReportImage(enrichmentBarChart, pathwayPath))
            {
                report.images.push_back({
                    "Pathway Enrichment", "pathway_enrichment_chart.png",
                    "Bars show -log10 adjusted p-values for ranked pathways."
                });
            }
        }

        report.notes = {
            "Statistical association does not by itself demonstrate biological "
            "causation; important results should be validated experimentally.",
            "Welch's t-test assumes independent biological samples and should "
            "not be used as a replacement for count-specific RNA-seq models.",
            "The built-in BioFlow pathway collection is an educational curated "
            "dataset and is not the complete official GO, KEGG or Reactome database.",
            "Tables in this report are limited to the first 100 genes and 30 pathways."
        };

        QString reportPath = reportDirectory.filePath("index.html");
        HtmlReportGenerator generator;
        generator.generate(reportPath.toStdString(), report);

        QMessageBox::information(
            this,
            "Report Generated",
            "The complete expression-analysis report was generated.\n\n"
            "Report: " + reportPath
        );

        QDesktopServices::openUrl(QUrl::fromLocalFile(reportPath));
    }
    catch (const std::exception& error)
    {
        QMessageBox::critical(
            this,
            "Report Generation Error",
            QString::fromStdString(error.what())
        );
    }
}

void MainWindow::generatePhylogeneticHtmlReport()
{
    const DistanceMatrix* availableMatrix = nullptr;

    if (currentDistanceMatrix.size() > 0)
    {
        availableMatrix = &currentDistanceMatrix;
    }
    else if (treeDistanceMatrix.size() > 0)
    {
        availableMatrix = &treeDistanceMatrix;
    }

    if (loadedSequences.empty()
        || (availableMatrix == nullptr && currentTree.isEmpty()))
    {
        QMessageBox::warning(
            this,
            "No Phylogenetic Results",
            "Generate a distance matrix or phylogenetic tree before creating "
            "the report."
        );
        return;
    }

    QString parentPath = QFileDialog::getExistingDirectory(
        this,
        "Select Folder for Phylogenetic Report",
        QString(),
        QFileDialog::ShowDirsOnly
    );

    if (parentPath.isEmpty())
    {
        return;
    }

    try
    {
        QString timeStamp = QDateTime::currentDateTime().toString(
            "yyyyMMdd_HHmmss"
        );
        QString folderName = "BioFlow_Phylogenetic_Report_" + timeStamp;
        QDir parentDirectory(parentPath);

        if (!parentDirectory.mkpath(folderName))
        {
            throw std::runtime_error("Could not create the report folder.");
        }

        QDir reportDirectory(parentDirectory.filePath(folderName));
        AnalysisReport report;
        report.title = "BioFlow Studio Phylogenetic Analysis Report";
        report.subtitle = "Sequence quality, pairwise distances and UPGMA tree";
        report.generatedAt = QDateTime::currentDateTime()
            .toString("dddd, dd MMMM yyyy — hh:mm AP")
            .toStdString();
        report.overview =
            "This automatically generated report summarizes the imported "
            "biological sequences, quality assessment, pairwise distance "
            "calculation and phylogenetic reconstruction.";

        QString method = !currentTree.isEmpty()
            ? treeAlignmentMethodBox->currentText()
            : matrixAlignmentMethodBox->currentText();

        report.metadata = {
            {"Input Files", std::to_string(selectedFastaFiles.size())},
            {"Sequences", std::to_string(loadedSequences.size())},
            {"Distance Method", method.toStdString()},
            {"Tree Method", currentTree.isEmpty()
                ? "Not generated" : "UPGMA"},
            {"Distance Matrix", availableMatrix == nullptr
                ? "Not generated" : "Available"}
        };

        std::vector<std::string> sourceFiles;

        for (const QString& filePath : selectedFastaFiles)
        {
            sourceFiles.push_back(filePath.toStdString());
        }

        SequenceQualityAnalyzer qualityAnalyzer(
            loadedSequences,
            sourceFiles
        );
        QualityReport qualityReport = qualityAnalyzer.analyze();
        ReportTable qualityTable;
        qualityTable.title = "FASTA Data-Quality Assessment";
        qualityTable.headers = {
            "Check", "Status", "Observation", "Recommendation"
        };

        for (const QualityCheck& check : qualityReport.getChecks())
        {
            qualityTable.rows.push_back({
                check.name,
                QualityReport::statusName(check.status),
                check.observation,
                check.recommendation
            });
        }

        report.tables.push_back(qualityTable);

        ReportTable sequenceTable;
        sequenceTable.title = "Imported Sequences";
        sequenceTable.headers = {"Identifier", "Type", "Length"};

        for (const auto& sequence : loadedSequences)
        {
            sequenceTable.rows.push_back({
                sequence->getIdentifier(),
                sequence->getTypeName(),
                std::to_string(sequence->getLength())
            });
        }

        report.tables.push_back(sequenceTable);

        if (availableMatrix != nullptr)
        {
            ReportTable matrixTable;
            matrixTable.title = "Pairwise Distance Matrix";
            matrixTable.headers.push_back("Sequence");

            for (const std::string& label : availableMatrix->getLabels())
            {
                matrixTable.headers.push_back(label);
            }

            for (std::size_t row = 0; row < availableMatrix->size(); ++row)
            {
                std::vector<std::string> values;
                values.push_back(availableMatrix->getLabels().at(row));

                for (std::size_t column = 0;
                     column < availableMatrix->size();
                     ++column)
                {
                    values.push_back(
                        QString::number(
                            availableMatrix->getDistance(row, column),
                            'f', 4
                        ).toStdString()
                    );
                }

                matrixTable.rows.push_back(values);
            }

            report.tables.push_back(matrixTable);
        }

        if (!currentTree.isEmpty())
        {
            ReportTable newickTable;
            newickTable.title = "Newick Tree Representation";
            newickTable.headers = {"Newick"};
            newickTable.rows = {{currentTree.toNewick()}};
            report.tables.push_back(newickTable);

            QString treePath = reportDirectory.filePath(
                "phylogenetic_tree.png"
            );

            if (saveReportImage(treeGraphic, treePath, 1200, 760))
            {
                report.images.push_back({
                    "UPGMA Phylogenetic Tree", "phylogenetic_tree.png",
                    "Branching summarizes similarity derived from the selected "
                    "pairwise distance strategy."
                });
            }
        }

        report.notes = {
            "UPGMA assumes an approximately constant evolutionary rate among "
            "lineages; this assumption may not hold for every dataset.",
            "Tree topology and branch lengths depend on sequence quality, "
            "alignment strategy and distance definition.",
            "This educational analysis should be confirmed with established "
            "phylogenetic software before research publication."
        };

        QString reportPath = reportDirectory.filePath("index.html");
        HtmlReportGenerator generator;
        generator.generate(reportPath.toStdString(), report);

        QMessageBox::information(
            this,
            "Report Generated",
            "The phylogenetic analysis report was generated.\n\n"
            "Report: " + reportPath
        );

        QDesktopServices::openUrl(QUrl::fromLocalFile(reportPath));
    }
    catch (const std::exception& error)
    {
        QMessageBox::critical(
            this,
            "Report Generation Error",
            QString::fromStdString(error.what())
        );
    }
}

void MainWindow::exportWidgetImage(
    QWidget* widget,
    const QString& suggestedFileName,
    const QString& dialogTitle
)
{
    if (widget == nullptr)
    {
        return;
    }

    QString filePath = QFileDialog::getSaveFileName(
        this,
        dialogTitle,
        suggestedFileName,
        "PNG Images (*.png)"
    );

    if (filePath.isEmpty())
    {
        return;
    }

    if (!filePath.endsWith(".png", Qt::CaseInsensitive))
    {
        filePath += ".png";
    }

    QPixmap image = widget->grab();

    if (image.isNull() || !image.save(filePath, "PNG"))
    {
        QMessageBox::critical(
            this,
            "Image Export Error",
            "The visualization could not be saved as a PNG image."
        );
        return;
    }

    QMessageBox::information(
        this,
        "Image Exported",
        "Visualization saved successfully:\n" + filePath
    );
}
