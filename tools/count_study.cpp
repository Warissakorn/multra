// Writes a counted four-leg study from tools/count_study_network.hpp, a count CSV and a site CSV.
// `countstudy` fails until studies/ratchada-latphrao/ matches what this writes.
//
//   multra-count-study <data-dir> <counts.csv> <site.csv> <id> <output.traffic.json>
#include "count_study_network.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
using namespace multra;
int main(int argc, char** argv) {
    if (argc != 6) {
        std::cerr << "usage: multra-count-study <data-dir> <counts.csv> <site.csv> <id> <output.traffic.json>\n";
        return 2;
    }
    try {
        const auto document = fixture::countStudy(argv[1], argv[2], argv[3], argv[4]);
        std::ofstream out(argv[5], std::ios::binary);
        out << documentJson(document).dump(2) << "\n";
        if (!out) { std::cerr << "could not write " << argv[5] << "\n"; return 1; }
    } catch (const std::exception& e) {
        std::cerr << e.what() << "\n";
        return 1;
    }
    return 0;
}
