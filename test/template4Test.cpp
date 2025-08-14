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

int main(int argc, char **argv)
{
    int port, party;
    parse_party_and_port(argv, &party, &port);
    NetIO *io = new NetIO(party == ALICE ? nullptr : "127.0.0.1", port);
    setup_semi_honest(io, party);

    const int num_cols = 3;
    const int num_rows = 1 << 4;

    SecureRelation someRel(num_cols, num_rows);
    std::vector<Stats> rel_stats;
    init_relation(someRel, rel_stats, num_cols, num_rows);

    SecureRelation instance1 = someRel;
    SecureRelation instance2 = someRel;

    int someValue = 4;
    int someOtherValue = 5;

    // =========================
    // Implementation A: Direct
    // =========================
    // auto start_A = std::chrono::high_resolution_clock::now();

    // FilterOperator filter1(2, Integer(32, someValue, ALICE), "eq");
    // SecureRelation filtered1 = filter1.execute(instance1);

    // FilterOperator filter2(2, Integer(32, someOtherValue, ALICE), "eq");
    // SecureRelation filtered2 = filter2.execute(instance2);

    // EquiJoinOperator joinA(1, 1);
    // SecureRelation resultA = joinA.execute(filtered1, filtered2);

    // auto end_A = std::chrono::high_resolution_clock::now();
    // std::cout << "=== Implementation A: Direct Filters + Join ===\n";
    // resultA.print_relation("Result A");
    // std::cout << "Time A: " << std::chrono::duration_cast<std::chrono::milliseconds>(end_A - start_A).count() << " ms\n\n";

    // =========================
    // Implementation B: Syscat + planNode
    // =========================
    auto start_B = std::chrono::high_resolution_clock::now();

    // Create FilterOperatorSyscat with stats
    FilterOperatorSyscat f1(2, Integer(32, someValue, ALICE), "eq", &rel_stats.at(2));
    FilterOperatorSyscat f2(2, Integer(32, someOtherValue, ALICE), "eq", &rel_stats.at(2));

    // Wrap filters in planNodes
    std::cout << "Wrap filters in planNodes\n";
    planNode select1(&f1, &instance1);
    planNode select2(&f2, &instance2);

    // Execute both filters
    std::cout << "Execute both filters\n";
    SecureRelation filteredB1 = select1.get_output();

    // for (int i = 0; i < filteredB1.columns[0].size(); i++)
    // {
    //     std::cout << filteredB1.columns[0][i] << "\n";
    // }

    SecureRelation filteredB2 = select2.get_output();

    // Join them
    std::cout << "Join\n";
    EquiJoinOperator joinB(1, 1);
    SecureRelation resultB = joinB.execute(filteredB1, filteredB2);

    std::cout << "Clock\n";
    auto end_B = std::chrono::high_resolution_clock::now();
    // std::cout << "=== Implementation B: Syscat Filters + planNode ===\n";
    resultB.print_relation("Result B");
    // std::cout << "Time B: " << std::chrono::duration_cast<std::chrono::milliseconds>(end_B - start_B).count() << " ms\n\n";

    io->flush();
    delete io;
    return 0;
}
