// Writes data/projects/yield-impatience-lab.traffic.json from tools/yield_impatience_network.hpp, its
// one source (YIELD_IMPATIENCE, D618). `yield_impatience` fails until the committed file matches.
//
//   multra-yield-impatience-fixture <data-dir> <output.traffic.json>
#include "yield_impatience_network.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
using namespace multra;
int main(int argc, char** argv) {
    if (argc != 3) { std::cerr << "usage: multra-yield-impatience-fixture <data-dir> <output.traffic.json>\n"; return 2; }
    std::ofstream out(argv[2], std::ios::binary);
    out << documentJson(fixture::yieldImpatienceLab(argv[1]).document).dump(2) << "\n";
    if (!out) { std::cerr << "could not write " << argv[2] << "\n"; return 1; }
    return 0;
}
