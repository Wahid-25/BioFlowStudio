#include "SampleGrouping.h"

#include <algorithm>
#include <cctype>
#include <stdexcept>

void SampleGrouping::automaticallyAssign(
    const std::vector<std::string>& sampleNames
)
{
    groups.assign(
        sampleNames.size(),
        SampleGroup::Unassigned
    );

    for (std::size_t index = 0;
         index < sampleNames.size();
         ++index)
    {
        std::string lowerName =
            sampleNames[index];

        std::transform(
            lowerName.begin(),
            lowerName.end(),
            lowerName.begin(),
            [](unsigned char character)
            {
                return static_cast<char>(
                    std::tolower(character)
                );
            }
        );

        if (lowerName.find("control")
            != std::string::npos)
        {
            groups[index] =
                SampleGroup::Control;
        }
        else if (
            lowerName.find("treatment")
            != std::string::npos
        )
        {
            groups[index] =
                SampleGroup::Treatment;
        }
    }
}

void SampleGrouping::setGroup(
    std::size_t sampleIndex,
    SampleGroup group
)
{
    if (sampleIndex >= groups.size())
    {
        throw std::out_of_range(
            "Sample-group index is outside its range."
        );
    }

    groups[sampleIndex] = group;
}

SampleGroup SampleGrouping::getGroup(
    std::size_t sampleIndex
) const
{
    if (sampleIndex >= groups.size())
    {
        throw std::out_of_range(
            "Sample-group index is outside its range."
        );
    }

    return groups[sampleIndex];
}

const std::vector<SampleGroup>&
SampleGrouping::getGroups() const
{
    return groups;
}

std::vector<std::size_t>
SampleGrouping::getControlIndices() const
{
    std::vector<std::size_t> indices;

    for (std::size_t index = 0;
         index < groups.size();
         ++index)
    {
        if (groups[index] == SampleGroup::Control)
        {
            indices.push_back(index);
        }
    }

    return indices;
}

std::vector<std::size_t>
SampleGrouping::getTreatmentIndices() const
{
    std::vector<std::size_t> indices;

    for (std::size_t index = 0;
         index < groups.size();
         ++index)
    {
        if (groups[index] == SampleGroup::Treatment)
        {
            indices.push_back(index);
        }
    }

    return indices;
}

std::size_t SampleGrouping::getControlCount() const
{
    return getControlIndices().size();
}

std::size_t SampleGrouping::getTreatmentCount() const
{
    return getTreatmentIndices().size();
}

bool SampleGrouping::isValid() const
{
    return getControlCount() >= 2
        && getTreatmentCount() >= 2;
}

void SampleGrouping::clear()
{
    groups.clear();
}