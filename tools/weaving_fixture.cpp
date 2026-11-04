// Writes data/projects/weaving-lab.traffic.json from tools/weaving_network.hpp, its one
// source (D449). `weavinglab` fails until the committed file matches, so the two cannot drift.
//
//   multra-weaving-fixture <output.traffic.json>
#include "weaving_network.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
using namespace multra;
int main(int argc, char** argv) {
    if (argc != 2) { std::cerr << "usage: multra-weaving-fixture <output.traffic.json>\n"; return 2; }
    std::ofstream out(argv[1], std::ios::binary);
    out << documentJson(fixture::weavingLab().document).dump(2) << "\n";
    if (!out) { std::cerr << "could not write " << argv[1] << "\n"; return 1; }
    return 0;
}
