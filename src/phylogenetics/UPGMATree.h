#pragma once

#include <memory>
#include <string>

#include "DistanceMatrix.h"

class UPGMATree
{
private:
    struct Node
    {
        std::string name;
        double height = 0.0;

        std::unique_ptr<Node> left;
        std::unique_ptr<Node> right;

        bool isLeaf() const;
    };

    std::unique_ptr<Node> root;

    std::string serializeNode(
        const Node& node,
        double parentHeight,
        bool isRoot
    ) const;

    std::string sanitizeName(
        const std::string& name
    ) const;

public:
    void build(const DistanceMatrix& matrix);

    bool isEmpty() const;
    std::string toNewick() const;
    void clear();
};