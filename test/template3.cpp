#include "emp-sh2pc/emp-sh2pc.h"
#include "core/relation.hpp"
#include "core/op_filter.hpp"
#include "core/op_equijoin.hpp"
#include "core/stats.hpp"
#include <iostream>
#include <map>
#include <vector>

using namespace emp;

// SQL: select * from r1, r2, r3, r4 where r1.c1 = r2.c1 and r1.c2 = r3.c2 and r2.c2 = r4.c2;

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

    SecureRelation r4(num_cols, num_rows);
    std::vector<Stats> r4_stats;
    init_relation(r4, r4_stats, num_cols, num_rows);

    // Step 1: r1 JOIN r2 ON r1.c1 = r2.c1 (col 1)
    EquiJoinOperator join1(1, 1);
    SecureRelation r1_r2 = join1.execute(r1, r2);

    // Step 2: r1_r2 JOIN r3 ON r1.c2 = r3.c2
    // r1 has 3 columns, so r1.c2 = column 2 in r1_r2
    // r3.c2 = column 2
    EquiJoinOperator join2(2, 2);
    SecureRelation r1_r2_r3 = join2.execute(r1_r2, r3);

    // Step 3: r1_r2_r3 JOIN r4 ON r2.c2 = r4.c2
    // r2 starts at col 3, r2.c2 = col 5 in r1_r2
    // So in r1_r2_r3, r2.c2 is still at index 5
    // r4.c2 = column 2
    EquiJoinOperator join3(5, 2);
    SecureRelation final_result = join3.execute(r1_r2_r3, r4);

    final_result.print_relation("Final Join Result:");

    io->flush();
    delete io;
    return 0;
}
