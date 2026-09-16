#include "ExpressionHeatmapWidget.h"

#include <QAbstractItemView>
#include <QColor>
#include <QHeaderView>
#include <QString>
#include <QStringList>
#include <QTableWidgetItem>

#include <algorithm>
#include <cmath>
#include <numeric>
#include <stdexcept>
#include <unordered_map>

namespace
{
QColor heatmapColor(double rowZScore)
{
    constexpr double maximumMagnitude = 2.5;

    double capped = std::clamp(
        rowZScore,
        -maximumMagnitude,
        maximumMagnitude
    );

    double intensity = std::abs(capped) / maximumMagnitude;

    QColor neutral("#FAFAFA");
    QColor target = capped < 0.0
        ? QColor("#2166AC")
        : QColor("#B2182B");

    int red = static_cast<int>(
        neutral.red()
        + intensity * (target.red() - neutral.red())
    );
    int green = static_cast<int>(
        neutral.green()
        + intensity * (target.green() - neutral.green())
    );
    int blue = static_cast<int>(
        neutral.blue()
        + intensity * (target.blue() - neutral.blue())
    );

    return QColor(red, green, blue);
}

QString groupName(SampleGroup group)
{
    if (group == SampleGroup::Control)
    {
        return "Control";
    }

    if (group == SampleGroup::Treatment)
    {
        return "Treatment";
    }

    return "Unassigned";
}
}

ExpressionHeatmapWidget::ExpressionHeatmapWidget(QWidget* parent)
    : QTableWidget(parent)
{
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setSelectionBehavior(QAbstractItemView::SelectItems);
    setAlternatingRowColors(false);
    setShowGrid(true);

    horizontalHeader()->setSectionResizeMode(
        QHeaderView::Interactive
    );
    horizontalHeader()->setDefaultSectionSize(105);

    verticalHeader()->setSectionResizeMode(
        QHeaderView::Fixed
    );
    verticalHeader()->setDefaultSectionSize(30);

    clearHeatmap();
}

void ExpressionHeatmapWidget::clearHeatmap()
{
    clear();
    setRowCount(0);
    setColumnCount(0);
}

void ExpressionHeatmapWidget::setData(
    const ExpressionDataset& dataset,
    const std::vector<std::vector<double>>& normalizedValues,
    const std::vector<DifferentialExpressionResult>& results,
    const SampleGrouping& grouping,
    std::size_t maximumGenes
)
{
    const std::size_t geneCount = dataset.getGeneCount();
    const std::size_t sampleCount = dataset.getSampleCount();

    if (normalizedValues.size() != geneCount)
    {
        throw std::invalid_argument(
            "Heatmap data does not match the number of genes."
        );
    }

    for (const auto& row : normalizedValues)
    {
        if (row.size() != sampleCount)
        {
            throw std::invalid_argument(
                "Heatmap row does not match the number of samples."
            );
        }
    }

    std::unordered_map<std::string, const DifferentialExpressionResult*>
        resultByGene;

    for (const DifferentialExpressionResult& result : results)
    {
        resultByGene[result.getGeneName()] = &result;
    }

    std::vector<std::size_t> geneIndices(geneCount);
    std::iota(geneIndices.begin(), geneIndices.end(), 0);

    const auto& geneNames = dataset.getGeneNames();

    std::stable_sort(
        geneIndices.begin(),
        geneIndices.end(),
        [&geneNames, &resultByGene](std::size_t left, std::size_t right)
        {
            const auto leftResult = resultByGene.find(
                geneNames.at(left)
            );
            const auto rightResult = resultByGene.find(
                geneNames.at(right)
            );

            double leftP = leftResult == resultByGene.end()
                ? 1.0
                : leftResult->second->getAdjustedPValue();
            double rightP = rightResult == resultByGene.end()
                ? 1.0
                : rightResult->second->getAdjustedPValue();

            return leftP < rightP;
        }
    );

    const std::size_t displayedGeneCount = std::min(
        maximumGenes,
        geneIndices.size()
    );

    clearHeatmap();
    setRowCount(static_cast<int>(displayedGeneCount));
    setColumnCount(static_cast<int>(sampleCount));

    QStringList horizontalLabels;
    const auto& sampleNames = dataset.getSampleNames();

    for (std::size_t sample = 0; sample < sampleCount; ++sample)
    {
        horizontalLabels.append(
            QString::fromStdString(sampleNames.at(sample))
            + "\n"
            + groupName(grouping.getGroup(sample))
        );
    }

    setHorizontalHeaderLabels(horizontalLabels);

    QStringList verticalLabels;

    for (std::size_t displayedRow = 0;
         displayedRow < displayedGeneCount;
         ++displayedRow)
    {
        std::size_t geneIndex = geneIndices.at(displayedRow);
        const std::string& geneName = geneNames.at(geneIndex);
        const std::vector<double>& values = normalizedValues.at(geneIndex);

        verticalLabels.append(QString::fromStdString(geneName));

        double mean = std::accumulate(
            values.begin(),
            values.end(),
            0.0
        ) / static_cast<double>(sampleCount);

        double squaredDifferenceSum = 0.0;

        for (double value : values)
        {
            double difference = value - mean;
            squaredDifferenceSum += difference * difference;
        }

        double standardDeviation = std::sqrt(
            squaredDifferenceSum
            / static_cast<double>(sampleCount)
        );

        const auto resultIterator = resultByGene.find(geneName);

        for (std::size_t sample = 0; sample < sampleCount; ++sample)
        {
            double rowZScore = standardDeviation > 1.0e-12
                ? (values.at(sample) - mean) / standardDeviation
                : 0.0;

            QTableWidgetItem* item = new QTableWidgetItem(
                QString::number(rowZScore, 'f', 2)
            );

            QColor background = heatmapColor(rowZScore);
            item->setBackground(background);
            item->setForeground(
                std::abs(rowZScore) >= 1.45
                    ? QColor(Qt::white)
                    : QColor("#203040")
            );
            item->setTextAlignment(Qt::AlignCenter);

            QString tooltip = QString(
                "Gene: %1\nSample: %2\n"
                "Normalized value: %3\nRow Z-score: %4"
            )
            .arg(QString::fromStdString(geneName))
            .arg(QString::fromStdString(sampleNames.at(sample)))
            .arg(values.at(sample), 0, 'g', 7)
            .arg(rowZScore, 0, 'f', 3);

            if (resultIterator != resultByGene.end())
            {
                tooltip += QString(
                    "\nAdjusted p-value: %1\nRegulation: %2"
                )
                .arg(
                    resultIterator->second->getAdjustedPValue(),
                    0,
                    'g',
                    7
                )
                .arg(
                    QString::fromStdString(
                        resultIterator->second->getRegulationName()
                    )
                );
            }

            item->setToolTip(tooltip);

            setItem(
                static_cast<int>(displayedRow),
                static_cast<int>(sample),
                item
            );
        }
    }

    setVerticalHeaderLabels(verticalLabels);
}
