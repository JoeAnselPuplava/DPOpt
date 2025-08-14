#ifndef RELATION_HPP
#define RELATION_HPP

#include "emp-sh2pc/emp-sh2pc.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <iostream>

class SecureRelation
{
public:
    std::vector<std::vector<emp::Integer>> columns;
    std::vector<emp::Integer> flags;
    std::vector<std::string> columnNames;

    SecureRelation() : SecureRelation(0, 0) {}
    SecureRelation(int column_count, int row_count);

    void sort_by_column(int column_index);
    void sort_by_flag();
    void sort_by_two_columns(int primary_column_index, int secondary_column_index);

    void bitonic_sort(int low, int high, bool ascending, std::vector<emp::Integer> &key_column);
    void bitonic_merge(int low, int high, bool ascending, std::vector<emp::Integer> &key_column);
    void swap_rows(int i, int j, emp::Bit condition);

    void sort_by_flag_goldreich();
    void goldreich_compaction(int low, int high);
    void goldreich_merge(int low, int mid, int high);

    void compact(int K);
    void print_relation(const std::string &label) const;
    void setColumnNames(const std::vector<std::string> &names);
    void addRow(const std::vector<std::string> &rowData); // no party param
};

// ---------------- Implementations ----------------

SecureRelation::SecureRelation(int column_count, int row_count)
{
    columns.resize(column_count, std::vector<emp::Integer>(row_count, emp::Integer(32, 0, emp::PUBLIC)));
    flags.resize(row_count, emp::Integer(1, 1, emp::PUBLIC));
}

void SecureRelation::sort_by_column(int column_index)
{
    if (column_index < 0 || column_index >= (int)columns.size())
    {
        std::cerr << "Error: Invalid column index!\n";
        return;
    }
    bitonic_sort(0, flags.size(), true, columns[column_index]);
}

void SecureRelation::sort_by_flag()
{
    bitonic_sort(0, flags.size(), true, flags);
}

void SecureRelation::bitonic_sort(int low, int high, bool ascending, std::vector<emp::Integer> &key_column)
{
    if (high <= 1)
        return;
    int mid = high / 2;
    bitonic_sort(low, mid, true, key_column);
    bitonic_sort(low + mid, mid, false, key_column);
    bitonic_merge(low, high, ascending, key_column);
}

void SecureRelation::bitonic_merge(int low, int high, bool ascending, std::vector<emp::Integer> &key_column)
{
    if (high <= 1)
        return;
    int mid = high / 2;
    for (int i = low; i < low + mid; i++)
    {
        emp::Bit condition = (key_column[i] > key_column[i + mid]) == ascending;
        swap_rows(i, i + mid, condition);
    }
    bitonic_merge(low, mid, ascending, key_column);
    bitonic_merge(low + mid, mid, ascending, key_column);
}

void SecureRelation::swap_rows(int i, int j, emp::Bit condition)
{
    for (auto &column : columns)
    {
        emp::Integer temp = column[i];
        column[i] = emp::If(condition, column[j], column[i]);
        column[j] = emp::If(condition, temp, column[j]);
    }
    emp::Integer temp_flag = flags[i];
    flags[i] = emp::If(condition, flags[j], flags[i]);
    flags[j] = emp::If(condition, temp_flag, flags[j]);
}

void SecureRelation::sort_by_flag_goldreich()
{
    goldreich_compaction(0, flags.size());
}

void SecureRelation::goldreich_compaction(int low, int high)
{
    if (high - low <= 1)
        return;
    int mid = (low + high) / 2;
    goldreich_compaction(low, mid);
    goldreich_compaction(mid, high);
    goldreich_merge(low, mid, high);
}

void SecureRelation::goldreich_merge(int low, int mid, int high)
{
    int i = mid - 1;
    int j = mid;
    while (i >= low && j < high)
    {
        emp::Bit condition = (flags[i] < flags[j]);
        swap_rows(i, j, condition);
        if (flags[i].reveal<int>() == 1)
            i--;
        if (flags[j].reveal<int>() == 1)
            j++;
    }
}

void SecureRelation::sort_by_two_columns(int primary_column_index, int secondary_column_index)
{
    if (primary_column_index < 0 || primary_column_index >= (int)columns.size() ||
        secondary_column_index < 0 || secondary_column_index >= (int)columns.size())
    {
        std::cerr << "Error: Invalid column index!\n";
        return;
    }
    sort_by_column(secondary_column_index);
    sort_by_column(primary_column_index);
}

void SecureRelation::compact(int K)
{
    sort_by_flag();
    if (!columns.empty() && columns[0].size() > (size_t)K)
    {
        for (auto &column : columns)
            column.resize(K);
        flags.resize(K);
    }
}

void SecureRelation::print_relation(const std::string &label) const
{
    std::cout << label << "\n";
    for (size_t row = 0; row < (columns.empty() ? 0 : columns[0].size()); ++row)
    {
        for (size_t col = 0; col < columns.size(); ++col)
        {
            std::cout << columns[col][row].reveal<int>() << "\t";
        }
        std::cout << "| Flag: " << flags[row].reveal<int>() << "\n";
    }
    std::cout << "\n";
}

void SecureRelation::setColumnNames(const std::vector<std::string> &names)
{
    columnNames = names;
    columns.clear();
    columns.resize(names.size());
}

void SecureRelation::addRow(const std::vector<std::string> &rowData)
{
    if (rowData.size() != columns.size())
    {
        std::cerr << "Error: rowData size doesn't match column count.\n";
        return;
    }
    for (size_t i = 0; i < rowData.size(); ++i)
    {
        int value = std::stoi(rowData[i]);
        columns[i].push_back(emp::Integer(32, value, emp::PUBLIC));
    }
    flags.push_back(emp::Integer(1, 1, emp::PUBLIC));
}

#endif // RELATION_HPP
