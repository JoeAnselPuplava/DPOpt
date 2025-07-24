#ifndef SCAN_OPERATOR_HPP
#define SCAN_OPERATOR_HPP

#include "relation.hpp"
#include <vector>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include "_op_unary.hpp"

class ScanOperator
{
public:
    // Store filename as a string, not pointer
    std::string csvFileName;

    // Constructor takes string by const reference
    ScanOperator(const std::string &filename) : csvFileName(filename) {}
    SecureRelation execute(const SecureRelation &input, const int party)
    {
        return operation(input, party);
    }

protected:
    SecureRelation operation(const SecureRelation &input, const int party)
    {

        if (csvFileName.empty())
        {
            std::cerr << "Error: CSV file name not provided.\n";
            return input;
        }

        std::ifstream file(csvFileName);
        if (!file.is_open())
        {
            std::cerr << "Error: Failed to open CSV file: " << csvFileName << std::endl;
            return input;
        }

        SecureRelation result;
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
                result.setColumnNames(rowData); // ensure this exists & is safe
                isHeader = false;
            }
            else
            {
                result.addRow(rowData, party); // ensure this exists & is safe
            }
        }

        file.close();
        return result;
    }
    // implement
    // 1. open the csv file with the name provided
    // optional: add field in SecureRelation object to maintain
    // mapping between column number and column name

    // 2. read contents

    // 3. place each row of data
    //  into the SecureRelation object passed in as parameter

    // placeholder return statement
};

#endif // SCAN_OPERATOR_HPP