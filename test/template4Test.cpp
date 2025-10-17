#include "emp-sh2pc/emp-sh2pc.h"
#include "core/relation.hpp"
#include "core/op_filter.hpp"
#include "core/op_equijoin.hpp"
#include "core/stats.hpp"
#include "core/op_filter_syscat.hpp"
#include "core/plan_node.hpp"
#include <iostream>
#include <map>
#include <vector>
#include <chrono>

using namespace emp;

void init_relation(SecureRelation &relation, std::vector<Stats> &rel_stats, int num_cols, int num_rows)
{
    for (int col = 0; col < num_cols; ++col)
    {
        std::map<int, int> freq_map;
        for (int row = 0; row < num_rows; ++row)
        {
            int val = rand() % 10;
            freq_map[val]++;
            relation.columns[col][row] = Integer(32, val, ALICE);
        }
        Stats s;
        s.column_index = col;
        for (auto &[key, freq] : freq_map)
        {
            s.mcv.push_back(key);
            s.mcf.push_back(freq);
        }
        s.num_rows = num_rows;
        rel_stats.push_back(s);
    }

    for (int row = 0; row < num_rows; ++row)
        relation.flags[row] = Integer(1, 1, ALICE);
}

void copy_relation(SecureRelation &sourceRel, SecureRelation &destRel, std::vector<Stats> &source_rel_stats, std::vector<Stats> &dest_rel_stats)
{
    // copy over column data
    for (int i = 0; i < sourceRel.columns.size(); i++)
    {
        for (int j = 0; j < sourceRel.columns.at(0).size(); j++)
        {
            destRel.columns.at(i).push_back(sourceRel.columns.at(i).at(j));
        }

        // copy over stats
        Stats s;
        s.column_index = source_rel_stats.at(i).column_index;
        for (int j = 0; j < source_rel_stats.at(i).mcv.size(); j++)
        {
            s.mcv.push_back(source_rel_stats.at(i).mcv.at(j));
            s.mcf.push_back(source_rel_stats.at(i).mcf.at(j));
        }
        s.num_rows = source_rel_stats.at(i).num_rows;
        dest_rel_stats.push_back(s);
    }

    // copy over flags
    for (int i = 0; i < sourceRel.flags.size(); i++)
    {
        destRel.flags.push_back(sourceRel.flags.at(i));
    }
}

int main(int argc, char **argv)
{
    int port, party;
    parse_party_and_port(argv, &party, &port);
    NetIO *io = new NetIO(party == ALICE ? nullptr : "127.0.0.1", port);
    setup_semi_honest(io, party);

    const int num_cols = 3;
    const int num_rows = 16;

    SecureRelation instance1(num_cols, num_rows);
    std::vector<Stats> rel_stats1;
    init_relation(instance1, rel_stats1, num_cols, num_rows);

    SecureRelation instance2(num_cols, 0);
    std::vector<Stats> rel_stats2;
    copy_relation(instance1, instance2, rel_stats1, rel_stats2);

    // SecureRelation instance1 = someRel;
    // SecureRelation instance2 = someRel;

    int someValue = 3;
    int someOtherValue = 5;

    // =========================
    // Implementation A: Direct
    // =========================
    auto start_A = std::chrono::high_resolution_clock::now();

    FilterOperator filter1(2, Integer(32, someValue, ALICE), "eq");
    SecureRelation filtered1 = filter1.execute(instance1);

    FilterOperator filter2(2, Integer(32, someOtherValue, ALICE), "eq");
    SecureRelation filtered2 = filter2.execute(instance2);

    EquiJoinOperator joinA(1, 1);
    SecureRelation resultA = joinA.execute(filtered1, filtered2);

    auto end_A = std::chrono::high_resolution_clock::now();
    // std::cout << "=== Implementation A: Direct Filters + Join ===\n";
    resultA.print_relation("Implementation A (FilterOperator) Result:");
    // std::cout << "Time A: " << std::chrono::duration_cast<std::chrono::milliseconds>(end_A - start_A).count() << " ms\n\n";

    // =========================
    // Implementation B: Syscat + planNode
    // =========================
    auto start_B = std::chrono::high_resolution_clock::now();

    // Create FilterOperatorSyscat with stats
    // instance1.print_relation("Original relation before filters: ");
    // instance2.print_relation("Copied relation before filters: ");
    FilterOperatorSyscat f1(2, Integer(32, 3, ALICE), "eq", &rel_stats1.at(2));
    FilterOperatorSyscat f2(2, Integer(32, 3, ALICE), "eq", &rel_stats2.at(2));
    // std::cout << "f1: " << f1.selectivity << "\n";
    // std::cout << "f2: " << f2.selectivity << "\n";
    // FilterOperatorSyscat f1(2, Integer(32, someValue, ALICE), "eq", &rel_stats.at(2));
    // FilterOperatorSyscat f2(2, Integer(32, someOtherValue, ALICE), "eq", &rel_stats.at(2));

    // Wrap filters in planNodes
    std::cout << "Wrap filters in planNodes\n";
    planNode select1(&f1, &instance1);
    planNode select2(&f2, &instance2);

    // Execute both filters
    std::cout << "Execute first filter\n";
    SecureRelation filteredB1 = select1.get_output();

    for (int i = 0; i < filteredB1.columns[0].size(); i++)
    {
        std::cout << filteredB1.columns[0][i] << "\n";
    }
    std::cout << "Execute second filter\n";
    SecureRelation filteredB2 = select2.get_output();

    // Join them
    std::cout << "Join\n";
    EquiJoinOperator joinB(1, 1);
    SecureRelation resultB = joinB.execute(filteredB1, filteredB2);

    std::cout << "Clock\n";
    auto end_B = std::chrono::high_resolution_clock::now();
    // std::cout << "=== Implementation B: Syscat Filters + planNode ===\n";
    resultB.print_relation("Implementation B (FilterOperatorSyscat + planNode) Result:");
    // std::cout << "Time B: " << std::chrono::duration_cast<std::chrono::milliseconds>(end_B - start_B).count() << " ms\n\n";

    io->flush();
    delete io;
    return 0;
}
