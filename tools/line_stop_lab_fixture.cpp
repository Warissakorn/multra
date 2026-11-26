// Writes data/projects/line-stop-lab.traffic.json from tools/line_stop_lab_network.hpp, its one source
// (LINE_SETBACK §8, D671). `linesetback` fails until the committed file matches.
//
//   multra-line-stop-lab-fixture <data-dir> <output.traffic.json>
#include "line_stop_lab_network.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
using namespace multra;
int main(int argc, char** argv) {
    if (argc != 3) { std::cerr << "usage: multra-line-stop-lab-fixture <data-dir> <output.traffic.json>\n"; return 2; }
    std::ofstream out(argv[2], std::ios::binary);
    out << documentJson(fixture::lineStopLab(argv[1])).dump(2) << "\n";
    if (!out) { std::cerr << "could not write " << argv[2] << "\n"; return 1; }
    return 0;
}
