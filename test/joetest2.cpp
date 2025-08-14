#include "emp-sh2pc/emp-sh2pc.h"
#include "core/op_scanner.hpp"
#include "core/relation.hpp"
#include <iostream>
#include "core/op_filter.hpp"
// #include "core/op_filter_syscat.hpp"
#include <chrono>

#include "core/op_equijoin.hpp"
#include "core/op_idx_equijoin.hpp" // Including the new header file
#include "core/stats.hpp"
#include <map>
#include "core/plan_node.hpp"

using namespace emp;

int main(int argc, char **argv)
{
    int port, party;
    parse_party_and_port(argv, &party, &port);

    NetIO *io = new NetIO(party == ALICE ? nullptr : "127.0.0.1", port);
    setup_semi_honest(io, party);
    std::string connStr = "dbname=dpopt user=joepuplava password= hostaddr=127.0.0.1 port=5432";
    std::string query = "SELECT * FROM my_table";
    ScanOperator pgScan(connStr, query);
    SecureRelation rel = pgScan.execute(SecureRelation());
    rel.print_relation("Postgres Output");
    io->flush();
    delete io;
    // finalize_plain_prot();

    return 0;
}
