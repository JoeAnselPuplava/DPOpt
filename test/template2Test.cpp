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

// SQL: select * from r1, r2, r3 where r1.c1 = r2.c1 and r2.c2 = r3.c2
void init_relation(SecureRelation &relation, std::vector<Stats> &rel_stats, int num_cols, int num_rows)
{
    for (int col = 0; col < num_cols; ++col)
    {
        std::map<int, int> value_count;
        for (int row = 0; row < num_rows; ++row)
        {
            int val = rand() % 10;
            value_count[val]++;
            relation.columns[col][row] = Integer(32, val, ALICE);
        }

        Stats s;
        s.column_index = col;
        for (auto &[key, freq] : value_count)
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

    SecureRelation r1(num_cols, num_rows);
    std::vector<Stats> r1_stats;
    init_relation(r1, r1_stats, num_cols, num_rows);

    SecureRelation r2(num_cols, num_rows);
    std::vector<Stats> r2_stats;
    init_relation(r2, r2_stats, num_cols, num_rows);

    SecureRelation r3(num_cols, num_rows);
    std::vector<Stats> r3_stats;
    init_relation(r3, r3_stats, num_cols, num_rows);

    // =========================
    // Implementation A: Original Joins
    // =========================
    // auto startA = std::chrono::high_resolution_clock::now();

    // // Step 1: r1 JOIN r2 on r1.c1 = r2.c1
    // EquiJoinOperator join1_A(1, 1);
    // SecureRelation r1_r2_A = join1_A.execute(r1, r2);

    // // Step 2: r1_r2 JOIN r3 on r2.c2 = r3.c2
    // EquiJoinOperator join2_A(5, 2);
    // SecureRelation final_result_A = join2_A.execute(r1_r2_A, r3);

    // auto endA = std::chrono::high_resolution_clock::now();
    // auto durationA = std::chrono::duration_cast<std::chrono::milliseconds>(endA - startA).count();

    // final_result_A.print_relation("Implementation A (Original) Final Join Result:");
    // std::cout << "Time A: " << durationA << " ms\n";

    // =========================
    // Implementation B: Syscat + planNode + Joins
    // =========================
    auto startB = std::chrono::high_resolution_clock::now();

    // Step 1: Filter-based approach before join (for demonstration)
    FilterOperatorSyscat f1(1, Integer(32, 3, ALICE), "eq", &r1_stats.at(1));
    planNode select1(&f1, &r1);
    SecureRelation r1_filtered = select1.get_output();

    FilterOperatorSyscat f2(1, Integer(32, 3, ALICE), "eq", &r2_stats.at(1));
    planNode select2(&f2, &r2);
    SecureRelation r2_filtered = select2.get_output();

    // Step 2: r1_filtered JOIN r2_filtered on r1.c1 = r2.c1
    EquiJoinOperator join1_B(1, 1);
    SecureRelation r1_r2_B = join1_B.execute(r1_filtered, r2_filtered);

    // Step 3: Join with r3 (filter optional here)
    FilterOperatorSyscat f3(2, Integer(32, 5, ALICE), "eq", &r3_stats.at(2));
    planNode select3(&f3, &r3);
    SecureRelation r3_filtered = select3.get_output();

    EquiJoinOperator join2_B(5, 2);
    SecureRelation final_result_B = join2_B.execute(r1_r2_B, r3_filtered);

    auto endB = std::chrono::high_resolution_clock::now();
    auto durationB = std::chrono::duration_cast<std::chrono::milliseconds>(endB - startB).count();

    final_result_B.print_relation("Implementation B (Syscat + planNode) Final Join Result:");
    // std::cout << "Time B: " << durationB << " ms\n";

    io->flush();
    delete io;
    return 0;
}
