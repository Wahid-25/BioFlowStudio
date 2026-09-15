#pragma once

#include <cstddef>
#include <string>
#include <vector>

enum class SampleGroup
{
    Unassigned,
    Control,
    Treatment
};

class SampleGrouping
{
private:
    std::vector<SampleGroup> groups;

public:
    void automaticallyAssign(
        const std::vector<std::string>& sampleNames
    );

    void setGroup(
        std::size_t sampleIndex,
        SampleGroup group
    );

    SampleGroup getGroup(
        std::size_t sampleIndex
    ) const;

    const std::vector<SampleGroup>&
    getGroups() const;

    std::vector<std::size_t>
    getControlIndices() const;

    std::vector<std::size_t>
    getTreatmentIndices() const;

    std::size_t getControlCount() const;
    std::size_t getTreatmentCount() const;

    bool isValid() const;
    void clear();
};