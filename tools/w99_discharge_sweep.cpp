// M3.3.3b BA231: queue discharge of the four-leg project at dt 0.1 / 0.25 / 0.5, prototype and w99,
// seeds 42-81 (docs/evidence/w99-discharge.md). Not in `check`: evidence to archive, like BA27's
// sweep, whose loop it shares. Sensitivity is recorded, never bounded; nothing here is calibration.
//
//   multra-w99-discharge-sweep <w99 behaviour.json> <project.traffic.json> <data dir> <out.csv>
#include "discharge_sweep.hpp"
#include <fstream>
using namespace multra;
int main(int argc, char** argv) {
    if (argc != 5) {
        std::cerr << "usage: multra-w99-discharge-sweep <w99 behaviour.json> <project> <data dir> <out.csv>\n";
        return 2;
    }
    try {
        const std::filesystem::path data = argv[3], out = argv[4];
        if (std::filesystem::exists(out)) throw std::invalid_argument("Output already exists: " + out.string());
        const auto w99 = parseBehaviour(w74fixture::read(argv[1]));
        if (!w99.w99) throw std::invalid_argument("The behaviour file must be a w99 behaviour");
        const auto base = w74fixture::base(argv[2], data);
        std::ofstream csv(out);
        sweep::discharge(base, data, "w99", w99, "W99 discharge fixture", csv);
        if (!csv) throw std::runtime_error("Failed writing " + out.string());
    } catch (const std::exception& e) { std::cerr << "error: " << e.what() << '\n'; return 1; }
}
