// Writes data/projects/lateral-lab.traffic.json from tools/lateral_lab_network.hpp, its one
// source (D457). `laterallab` fails until the committed file matches, so the two cannot drift.
//
//   multra-lateral-fixture <data-dir> <output.traffic.json>
#include "lateral_lab_network.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
using namespace multra;
int main(int argc, char** argv) {
    if (argc != 3) { std::cerr << "usage: multra-lateral-fixture <data-dir> <output.traffic.json>\n"; return 2; }
    std::ofstream out(argv[2], std::ios::binary);
    out << documentJson(fixture::lateralLab(argv[1]).document).dump(2) << "\n";
    if (!out) { std::cerr << "could not write " << argv[2] << "\n"; return 1; }
    return 0;
}
