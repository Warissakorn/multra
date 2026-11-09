// Writes data/projects/macro-four-step.traffic.json from tools/macro_example_network.hpp, its one
// source. `macroexample` fails until the committed file matches, so the two cannot drift.
//
//   multra-macro-example <data-dir> <output.traffic.json>
#include "macro_example_network.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
using namespace multra;
int main(int argc, char** argv) {
    if (argc != 3) { std::cerr << "usage: multra-macro-example <data-dir> <output.traffic.json>\n"; return 2; }
    std::ofstream out(argv[2], std::ios::binary);
    out << documentJson(fixture::macroExample(argv[1]).document).dump(2) << "\n";
    if (!out) { std::cerr << "could not write " << argv[2] << "\n"; return 1; }
    return 0;
}
