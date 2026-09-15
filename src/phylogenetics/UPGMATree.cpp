#include "UPGMATree.h"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vector>

bool UPGMATree::Node::isLeaf() const
{
    return !left && !right;
}

void UPGMATree::build(const DistanceMatrix& matrix)
{
    if (matrix.size() < 2)
    {
        throw std::invalid_argument(
            "At least two sequences are required to build a tree."
        );
    }

    struct Cluster
    {
        std::unique_ptr<Node> node;
        std::vector<std::size_t> members;
    };

    clear();

    std::vector<Cluster> clusters;
    const auto& labels = matrix.getLabels();
    const auto& distances = matrix.getValues();

    for (std::size_t index = 0;
         index < labels.size();
         ++index)
    {
        Cluster cluster;

        cluster.node = std::make_unique<Node>();
        cluster.node->name = labels[index];
        cluster.node->height = 0.0;
        cluster.members.push_back(index);

        clusters.push_back(std::move(cluster));
    }

    auto averageDistance =
        [&distances](
            const Cluster& first,
            const Cluster& second
        )
        {
            double total = 0.0;
            std::size_t comparisons = 0;

            for (std::size_t firstMember : first.members)
            {
                for (std::size_t secondMember : second.members)
                {
                    total += distances[firstMember][secondMember];
                    ++comparisons;
                }
            }

            return total /
                static_cast<double>(comparisons);
        };

    while (clusters.size() > 1)
    {
        double smallestDistance =
            std::numeric_limits<double>::max();

        std::size_t firstIndex = 0;
        std::size_t secondIndex = 1;

        for (std::size_t first = 0;
             first < clusters.size();
             ++first)
        {
            for (std::size_t second = first + 1;
                 second < clusters.size();
                 ++second)
            {
                double distance = averageDistance(
                    clusters[first],
                    clusters[second]
                );

                if (distance < smallestDistance)
                {
                    smallestDistance = distance;
                    firstIndex = first;
                    secondIndex = second;
                }
            }
        }

        Cluster mergedCluster;

        mergedCluster.node = std::make_unique<Node>();
        mergedCluster.node->height =
            smallestDistance / 2.0;

        mergedCluster.node->left =
            std::move(clusters[firstIndex].node);

        mergedCluster.node->right =
            std::move(clusters[secondIndex].node);

        mergedCluster.members =
            clusters[firstIndex].members;

        mergedCluster.members.insert(
            mergedCluster.members.end(),
            clusters[secondIndex].members.begin(),
            clusters[secondIndex].members.end()
        );

        clusters.erase(
            clusters.begin() + secondIndex
        );

        clusters.erase(
            clusters.begin() + firstIndex
        );

        clusters.push_back(
            std::move(mergedCluster)
        );
    }

    root = std::move(clusters.front().node);
}

bool UPGMATree::isEmpty() const
{
    return root == nullptr;
}

std::string UPGMATree::toNewick() const
{
    if (!root)
    {
        throw std::runtime_error(
            "The phylogenetic tree has not been generated."
        );
    }

    return serializeNode(
        *root,
        root->height,
        true
    ) + ";";
}

void UPGMATree::clear()
{
    root.reset();
}

std::string UPGMATree::serializeNode(
    const Node& node,
    double parentHeight,
    bool isRoot
) const
{
    std::string result;

    if (node.isLeaf())
    {
        result = sanitizeName(node.name);
    }
    else
    {
        result =
            "("
            + serializeNode(
                *node.left,
                node.height,
                false
            )
            + ","
            + serializeNode(
                *node.right,
                node.height,
                false
            )
            + ")";
    }

    if (!isRoot)
    {
        double branchLength =
            std::max(0.0, parentHeight - node.height);

        std::ostringstream stream;
        stream << std::fixed
               << std::setprecision(6)
               << branchLength;

        result += ":" + stream.str();
    }

    return result;
}

std::string UPGMATree::sanitizeName(
    const std::string& name
) const
{
    std::string sanitized = name;

    for (char& character : sanitized)
    {
        if (std::isspace(
                static_cast<unsigned char>(character)
            )
            || character == '('
            || character == ')'
            || character == ':'
            || character == ','
            || character == ';')
        {
            character = '_';
        }
    }

    return sanitized;
}