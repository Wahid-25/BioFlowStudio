#include "ExpressionParser.h"

#include <cctype>
#include <fstream>
#include <stdexcept>
#include <unordered_set>

ExpressionDataset ExpressionParser::parseFile(
    const std::string& filePath
) const
{
    std::ifstream input(filePath);

    if (!input.is_open())
    {
        throw std::runtime_error(
            "Could not open expression file: " + filePath
        );
    }

    std::string headerLine;

    if (!std::getline(input, headerLine))
    {
        throw std::runtime_error(
            "The expression file is empty."
        );
    }

    char delimiter = detectDelimiter(headerLine);

    std::vector<std::string> header =
        splitLine(headerLine, delimiter);

    if (header.size() < 3)
    {
        throw std::runtime_error(
            "The expression file must contain a gene "
            "column and at least two sample columns."
        );
    }

    std::vector<std::string> sampleNames(
        header.begin() + 1,
        header.end()
    );

    std::unordered_set<std::string> sampleCheck;

    for (std::string& sample : sampleNames)
    {
        sample = trim(sample);

        if (sample.empty())
        {
            throw std::runtime_error(
                "An empty sample name was found."
            );
        }

        if (!sampleCheck.insert(sample).second)
        {
            throw std::runtime_error(
                "Duplicate sample name: " + sample
            );
        }
    }

    std::vector<std::string> geneNames;
    std::vector<std::vector<double>> values;

    std::string line;
    std::size_t lineNumber = 1;

    while (std::getline(input, line))
    {
        ++lineNumber;

        if (trim(line).empty())
        {
            continue;
        }

        std::vector<std::string> fields =
            splitLine(line, delimiter);

        if (fields.size() != header.size())
        {
            throw std::runtime_error(
                "Incorrect number of columns at line "
                + std::to_string(lineNumber)
            );
        }

        std::string geneName = trim(fields[0]);

        if (geneName.empty())
        {
            throw std::runtime_error(
                "Missing gene name at line "
                + std::to_string(lineNumber)
            );
        }

        std::vector<double> rowValues;

        for (std::size_t column = 1;
             column < fields.size();
             ++column)
        {
            std::string valueText =
                trim(fields[column]);

            if (valueText.empty())
            {
                throw std::runtime_error(
                    "Missing expression value at line "
                    + std::to_string(lineNumber)
                );
            }

            try
            {
                std::size_t processedCharacters = 0;

                double value = std::stod(
                    valueText,
                    &processedCharacters
                );

                if (processedCharacters
                    != valueText.length())
                {
                    throw std::invalid_argument(
                        "Additional characters found"
                    );
                }

                rowValues.push_back(value);
            }
            catch (const std::exception&)
            {
                throw std::runtime_error(
                    "Invalid numerical value at line "
                    + std::to_string(lineNumber)
                    + ", column "
                    + std::to_string(column + 1)
                );
            }
        }

        geneNames.push_back(geneName);
        values.push_back(rowValues);
    }

    ExpressionDataset dataset(
        "Imported Expression Dataset",
        filePath
    );

    dataset.setData(
        sampleNames,
        geneNames,
        values
    );

    if (!dataset.validate())
    {
        throw std::runtime_error(
            "Expression dataset validation failed. "
            "Check duplicate genes, negative values, "
            "sample names and row lengths."
        );
    }

    return dataset;
}

char ExpressionParser::detectDelimiter(
    const std::string& headerLine
) const
{
    std::size_t commaCount = 0;
    std::size_t tabCount = 0;

    for (char character : headerLine)
    {
        if (character == ',')
        {
            ++commaCount;
        }
        else if (character == '\t')
        {
            ++tabCount;
        }
    }

    if (commaCount == 0 && tabCount == 0)
    {
        throw std::runtime_error(
            "Could not detect CSV or TSV delimiter."
        );
    }

    return tabCount > commaCount ? '\t' : ',';
}

std::vector<std::string> ExpressionParser::splitLine(
    const std::string& line,
    char delimiter
) const
{
    std::vector<std::string> fields;
    std::string currentField;
    bool insideQuotes = false;

    for (std::size_t index = 0;
         index < line.length();
         ++index)
    {
        char character = line[index];

        if (character == '"')
        {
            if (insideQuotes
                && index + 1 < line.length()
                && line[index + 1] == '"')
            {
                currentField += '"';
                ++index;
            }
            else
            {
                insideQuotes = !insideQuotes;
            }
        }
        else if (
            character == delimiter
            && !insideQuotes
        )
        {
            fields.push_back(
                trim(currentField)
            );

            currentField.clear();
        }
        else if (character != '\r')
        {
            currentField += character;
        }
    }

    if (insideQuotes)
    {
        throw std::runtime_error(
            "An unclosed quotation mark was found."
        );
    }

    fields.push_back(trim(currentField));

    return fields;
}

std::string ExpressionParser::trim(
    const std::string& text
) const
{
    std::size_t beginning = 0;

    while (beginning < text.length()
           && std::isspace(
               static_cast<unsigned char>(
                   text[beginning]
               )
           ))
    {
        ++beginning;
    }

    std::size_t ending = text.length();

    while (ending > beginning
           && std::isspace(
               static_cast<unsigned char>(
                   text[ending - 1]
               )
           ))
    {
        --ending;
    }

    return text.substr(
        beginning,
        ending - beginning
    );
}