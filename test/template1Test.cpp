#include "emp-sh2pc/emp-sh2pc.h"
#include "core/relation.hpp"
#include "core/op_filter.hpp"
#include "core/op_filter_syscat.hpp"

#include "core/op_equijoin.hpp"
#include "core/plan_node.hpp"
#include <iostream>
#include <chrono>
#include <map>
#include <vector>
#include "core/stats.hpp"

using namespace emp;

void init_relation(SecureRelation &relation, std::vector<Stats> &rel_stats, int num_cols, int num_rows)
{
    for (int col = 0; col < num_cols; ++col)
    {
        std::map<int, int> myMap;
        for (int row = 0; row < num_rows; ++row)
        {
            int val = rand() % 10;
            if (myMap.count(val) == 0)
                myMap[val] = 0;
            myMap[val]++;
            relation.columns[col][row] = Integer(32, val, ALICE);
        }
        Stats s;
        s.column_index = col;
        for (auto &[key, value] : myMap)
        {
            s.mcv.push_back(key);
            s.mcf.push_back(value);
        }
        s.num_rows = num_rows;
        rel_stats.push_back(s);
    }

    for (int row = 0; row < num_rows; ++row)
        relation.flags[row] = Integer(1, 1, ALICE);
}

int main(int argc, char **argv)
{
    int port, party;
    parse_party_and_port(argv, &party, &port);

    NetIO *io = new NetIO(party == ALICE ? nullptr : "127.0.0.1", port);
    setup_semi_honest(io, party);

    const int num_cols = 3;
    const int num_rows = 1 << 4;

    SecureRelation r1(num_cols, num_rows);
    std::vector<Stats> r1_stats;
    init_relation(r1, r1_stats, num_cols, num_rows);

    SecureRelation r2(num_cols, num_rows);
    std::vector<Stats> r2_stats;
    init_relation(r2, r2_stats, num_cols, num_rows);

    int someValue = 3;

    // =========================
    // Implementation A: Original
    // =========================

    auto startA = std::chrono::high_resolution_clock::now();

    FilterOperator filter_r1(1, Integer(32, someValue, ALICE), "eq");
    SecureRelation r1_filtered = filter_r1.execute(r1);

    FilterOperator filter_r2(1, Integer(32, someValue, ALICE), "eq");
    SecureRelation r2_filtered = filter_r2.execute(r2);

    EquiJoinOperator joinA(1, 1);
    SecureRelation resultA = joinA.execute(r1_filtered, r2_filtered);

    auto endA = std::chrono::high_resolution_clock::now();
    auto durationA = std::chrono::duration_cast<std::chrono::milliseconds>(endA - startA).count();

    resultA.print_relation("Implementation A (FilterOperator) Result:");
    // std::cout << "Time A: " << durationA << " ms\n";

    // =========================
    // Implementation B: Syscat + planNode
    // =========================

    auto startB = std::chrono::high_resolution_clock::now();

    // Create FilterOperatorSyscat using stats
    FilterOperatorSyscat f1(1, Integer(32, someValue, ALICE), "eq", &r1_stats.at(1));
    planNode select1(&f1, &r1);

    // Execute planNode (filter)
    SecureRelation r1_syscat_filtered = select1.get_output();

    // Second filter
    FilterOperatorSyscat f2(1, Integer(32, someValue, ALICE), "eq", &r2_stats.at(1));
    planNode select2(&f2, &r2);
    SecureRelation r2_syscat_filtered = select2.get_output();

    // Join
    EquiJoinOperator joinB(1, 1);
    SecureRelation resultB = joinB.execute(r1_syscat_filtered, r2_syscat_filtered);

    auto endB = std::chrono::high_resolution_clock::now();
    auto durationB = std::chrono::duration_cast<std::chrono::milliseconds>(endB - startB).count();

    resultB.print_relation("Implementation B (FilterOperatorSyscat + planNode) Result:");
    // std::cout << "Time B: " << durationB << " ms\n";

    io->flush();
    delete io;
    return 0;
}
