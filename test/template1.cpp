#include "emp-sh2pc/emp-sh2pc.h"
#include "core/relation.hpp"
#include "core/op_filter.hpp"
#include "core/op_equijoin.hpp"
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

    // ---- Query Plan ----
    // SQL: select * from r1 join r2 on r1.c1 = r2.c1 where r1.c1 = someValue and r2.c1 = someValue;
    int someValue = 3;

    // Filter r1.c1 = someValue
    FilterOperator filter_r1(1, Integer(32, someValue, ALICE), "eq");
    // FilterOperatorSyscat filter_r1(1, Integer(32, someValue, ALICE), "eq");
    // planNode select1(&f1, &rel1);
    SecureRelation r1_filtered = filter_r1.execute(r1);

    // Filter r2.c1 = someValue
    FilterOperator filter_r2(1, Integer(32, someValue, ALICE), "eq");
    SecureRelation r2_filtered = filter_r2.execute(r2);

    // Join on c1 = c1 (i.e., column 1 in both relations)
    EquiJoinOperator join(1, 1);
    SecureRelation result = join.execute(r1_filtered, r2_filtered);

    result.print_relation("Query Result:");

    io->flush();
    delete io;
    return 0;
}
