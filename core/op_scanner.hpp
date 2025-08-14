#ifndef SCAN_OPERATOR_HPP
#define SCAN_OPERATOR_HPP

#include "relation.hpp"
#include <vector>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include "_op_unary.hpp"

// Optional: Include libpqxx for PostgreSQL support
#include <pqxx/pqxx>

class ScanOperator : public UnaryOperator
{
public:
    enum class SourceType
    {
        CSV,
        POSTGRES
    };

    // Constructor for CSV
    explicit ScanOperator(const std::string &filename)
        : source(SourceType::CSV), csvFileName(filename) {}

    // Constructor for PostgreSQL
    ScanOperator(const std::string &connStr, const std::string &query)
        : source(SourceType::POSTGRES), pgConnStr(connStr), pgQuery(query) {}

protected:
    SecureRelation operation(const SecureRelation & /*input*/) override
    {
        SecureRelation result;

        if (source == SourceType::CSV)
        {
            return scanCSV(result);
        }
        else if (source == SourceType::POSTGRES)
        {
            return scanPostgres(result);
        }

        return result;
    }

private:
    SourceType source;
    std::string csvFileName;
    std::string pgConnStr;
    std::string pgQuery;

    SecureRelation scanCSV(SecureRelation &result)
    {
        if (csvFileName.empty())
        {
            std::cerr << "[ScanOperator] CSV filename not provided.\n";
            return result;
        }

        std::ifstream file(csvFileName);
        if (!file.is_open())
        {
            std::cerr << "[ScanOperator] Failed to open CSV file: " << csvFileName << "\n";
            return result;
        }

        std::string line;
        bool isHeader = true;

        while (std::getline(file, line))
        {
            std::stringstream ss(line);
            std::string cell;
            std::vector<std::string> rowData;

            while (std::getline(ss, cell, ','))
            {
                rowData.push_back(cell);
            }

            if (isHeader)
            {
                result.setColumnNames(rowData);
                isHeader = false;
            }
            else
            {
                result.addRow(rowData);
            }
        }

        file.close();
        return result;
    }

    SecureRelation scanPostgres(SecureRelation &result)
    {
        try
        {
            pqxx::connection c(pgConnStr);
            pqxx::work txn(c);

            pqxx::result r = txn.exec(pgQuery);

            if (r.empty())
                return result;

            // set column names
            std::vector<std::string> colNames;
            for (pqxx::row_size_type i = 0; i < r.columns(); ++i) // use index loop
            {
                colNames.push_back(r.column_name(i));
            }
            result.setColumnNames(colNames);

            // add rows
            for (pqxx::result::const_iterator rowIt = r.begin(); rowIt != r.end(); ++rowIt)
            {
                std::vector<std::string> rowData;
                for (pqxx::row::size_type j = 0; j < rowIt->size(); ++j)
                {
                    rowData.push_back((*rowIt)[j].c_str());
                }
                result.addRow(rowData);
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "[ScanOperator] PostgreSQL error: " << e.what() << "\n";
        }

        return result;
    }
};

#endif // SCAN_OPERATOR_HPP
