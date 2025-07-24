#include "emp-sh2pc/emp-sh2pc.h"
#include "core/relation.hpp"
#include "core/op_filter.hpp"
#include "core/op_equijoin.hpp"
#include "core/stats.hpp"
#include <iostream>
#include <map>
#include <vector>

using namespace emp;

// SQL: select * from someRel instance1 join someRel instance2
//      on instance1.c1 = instace2.c1
//      where instance1.c2 = someValue and
//      instance2.c2 = someOtherValue

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

    // same relation, used twice
    SecureRelation someRel(num_cols, num_rows);
    std::vector<Stats> rel_stats;
    init_relation(someRel, rel_stats, num_cols, num_rows);

    // simulate instance1 and instance2 as two copies of sameRel
    SecureRelation instance1 = someRel;
    SecureRelation instance2 = someRel;

    int someValue = 4;
    int someOtherValue = 5;

    // Step 1: Filter instance1 on c2 = someValue
    FilterOperator filter1(2, Integer(32, someValue, ALICE), "eq");
    SecureRelation filtered1 = filter1.execute(instance1);

    // Step 2: Filter instance2 on c2 = someOtherValue
    FilterOperator filter2(2, Integer(32, someOtherValue, ALICE), "eq");
    SecureRelation filtered2 = filter2.execute(instance2);

    // Step 3: Join on c1 = c1 (i.e., column 1 in both relations)
    EquiJoinOperator join(1, 1);
    SecureRelation result = join.execute(filtered1, filtered2);

    result.print_relation("Self Join Result with Filters:");

    io->flush();
    delete io;
    return 0;
}
