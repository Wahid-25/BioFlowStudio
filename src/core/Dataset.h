#pragma once

#include <string>

enum class DatasetStatus
{
    NotLoaded,
    Loaded,
    Valid,
    Invalid
};

class Dataset
{
private:
    std::string name;
    std::string sourceFile;
    DatasetStatus status;

protected:
    void setStatus(DatasetStatus status);

public:
    Dataset(
        const std::string& name,
        const std::string& sourceFile
    );

    virtual ~Dataset() = default;

    const std::string& getName() const;
    const std::string& getSourceFile() const;
    DatasetStatus getStatus() const;

    void setName(const std::string& name);
    void setSourceFile(const std::string& sourceFile);

    std::string getStatusName() const;

    virtual std::string getTypeName() const = 0;
    virtual bool validate() = 0;
};