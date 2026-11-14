// Writes data/projects/m6-s1-approach.traffic.json from tools/m6_s1_network.hpp, the project's one
// source (D535, M6_S1_RUN §1). `m6s1` (MS1) fails until the committed file matches.
//
//   multra-m6-s1-fixture <output.traffic.json>
#include "m6_s1_network.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
using namespace multra;
int main(int argc, char** argv) {
    if (argc != 2) { std::cerr << "usage: multra-m6-s1-fixture <output.traffic.json>\n"; return 2; }
    std::ofstream out(argv[1], std::ios::binary);
    out << documentJson(m6s1::approach()).dump(2) << "\n";
    if (!out) { std::cerr << "could not write " << argv[1] << "\n"; return 1; }
    return 0;
}
