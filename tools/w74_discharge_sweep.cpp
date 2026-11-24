// M3.3.3a BA27: queue discharge of the four-leg project at dt 0.1 / 0.25 / 0.5, prototype and w74,
// seeds 42-81 (docs/evidence/w74-discharge.md). Not in `check`: evidence to archive, like the
// other sweeps. Sensitivity is recorded, never bounded; nothing here is calibration (rule 4).
//
//   multra-w74-discharge-sweep <w74 behaviour.json> <project.traffic.json> <data dir> <out.csv>
//
// Both models run on the same project with its catalogs captured as owned; the w74 arm adds the
// given behaviour and points every vehicle type at it. The loop is discharge_sweep.hpp (D649).
#include "discharge_sweep.hpp"
#include <fstream>
using namespace multra;
int main(int argc, char** argv) {
    if (argc != 5) {
        std::cerr << "usage: multra-w74-discharge-sweep <w74 behaviour.json> <project> <data dir> <out.csv>\n";
        return 2;
    }
    try {
        const std::filesystem::path data = argv[3], out = argv[4];
        if (std::filesystem::exists(out)) throw std::invalid_argument("Output already exists: " + out.string());
        const auto w74 = w74fixture::behaviour(argv[1]);
        const auto base = w74fixture::base(argv[2], data);
        std::ofstream csv(out);
        sweep::discharge(base, data, "w74", w74, "W74 discharge fixture", csv);
        if (!csv) throw std::runtime_error("Failed writing " + out.string());
    } catch (const std::exception& e) { std::cerr << "error: " << e.what() << '\n'; return 1; }
}
