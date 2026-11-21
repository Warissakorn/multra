// Writes data/projects/priority-rule-lab.traffic.json from tools/priority_rule_network.hpp, its one
// source (PRIORITY_RULES, D614-D616). `priority_marker` fails until the committed file matches.
//
//   multra-priority-rule-fixture <output.traffic.json>
#include "priority_rule_network.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
using namespace multra;
int main(int argc, char** argv) {
    if (argc != 2) { std::cerr << "usage: multra-priority-rule-fixture <output.traffic.json>\n"; return 2; }
    std::ofstream out(argv[1], std::ios::binary);
    out << documentJson(fixture::priorityRuleLab().document).dump(2) << "\n";
    if (!out) { std::cerr << "could not write " << argv[1] << "\n"; return 1; }
    return 0;
}
