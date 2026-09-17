#pragma once

#include <string>
#include <vector>

struct PathwayRecord
{
    std::string id;
    std::string name;
    std::string category;
    std::vector<std::string> genes;
};

class PathwayDatabase
{
public:
    virtual ~PathwayDatabase() = default;
    virtual std::string getName() const = 0;
    virtual std::vector<PathwayRecord> getPathways() const = 0;
};

class BuiltInPathwayDatabase : public PathwayDatabase
{
public:
    std::string getName() const override;
    std::vector<PathwayRecord> getPathways() const override;
};
