// D449: the weaving sweep (docs/reference/WEAVING_FIXTURE.md §4-§5) -- each scene of the weaving
// lab across demand factors, lateral off and on, and seeds, printed as markdown. Not in `check`:
// development evidence, like the other sweeps. Nothing here is validated (rule 4).
//
//   multra-weaving-sweep <repo root> [--seeds 42-51] [--factors 0.6,0.8,1.0,1.2,1.4]
#include "weaving_network.hpp"
#include "weaving_sweep.hpp"
#include "../src/project/json.hpp"
#include <nlohmann/json.hpp>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
using namespace multra;
namespace {
constexpr double kDemandEnd = 600; // s: the lab's inputs end here (tools/weaving_network.hpp)
std::string fixed(double v, int places = 1) { std::ostringstream o; o << std::fixed << std::setprecision(places) << v; return o.str(); }
std::vector<double> numbers(const std::string& text) {
    std::vector<double> out; std::stringstream in(text);
    for (std::string item; std::getline(in, item, ',');) out.push_back(std::stod(item));
    return out;
}
// One scene's figures in one run: movements and lane-change rows are named "<first> → <last>",
// and every first Link of a scene starts with the scene's name.
struct Scene { double served{}, unfinished{}, delaySum{}, changes{}, waiting{}, longest{}, last{}; };
Scene scene(const sweep::WeavingRun& r, const std::string& name, const fixture::WeavingScene& lab) {
    Scene s;
    for (const auto& id : {lab.toMain, lab.toExit})
        if (const auto it = r.lastArrival.find(id); it != r.lastArrival.end()) s.last = std::max(s.last, it->second);
    for (const auto& m : r.movements.movements) {
        if (m.name.rfind(name + " ", 0) != 0) continue;
        s.served += static_cast<double>(m.vehicles); s.unfinished += static_cast<double>(m.unfinished);
        s.delaySum += m.meanDelay.value_or(0) * static_cast<double>(m.vehicles);
    }
    for (const auto& c : r.changes.rows) {
        if (c.name.rfind(name + " ", 0) != 0) continue;
        s.changes += static_cast<double>(c.changes); s.waiting += static_cast<double>(c.waitingVehicles);
        s.longest = std::max(s.longest, c.longestWait);
    }
    return s;
}
}
int main(int argc, char** argv) {
    try {
        if (argc < 2 || argc % 2) {
            std::cerr << "usage: multra-weaving-sweep <repo root> [--seeds 42-51] [--factors 0.6,0.8,1.0,1.2,1.4]\n";
            return 2;
        }
        const std::filesystem::path root = argv[1];
        std::uint32_t first = 42, last = 51;
        std::vector<double> factors{0.6, 0.8, 1.0, 1.2, 1.4};
        for (int i = 2; i < argc; i += 2) {
            const std::string flag = argv[i], value = argv[i + 1];
            if (flag == "--seeds") {
                const auto dash = value.find('-');
                first = static_cast<std::uint32_t>(std::stoul(value.substr(0, dash)));
                last = dash == std::string::npos ? first : static_cast<std::uint32_t>(std::stoul(value.substr(dash + 1)));
            } else if (flag == "--factors") factors = numbers(value);
            else throw std::invalid_argument("unknown option " + flag);
        }
        if (last < first) throw std::invalid_argument("empty seed range");
        const auto project = root / "data/projects/weaving-lab.traffic.json";
        std::ifstream stream(project);
        if (!stream) throw std::runtime_error("Cannot read project: " + project.string());
        const auto document = parseDocument(Json::parse(stream));
        const auto data = root / "data";
        const std::vector<std::string> names{"Weave 150 m", "Weave 300 m", "Weave 500 m"};
        const auto lab = fixture::weavingLab(); // the committed file's builder: the same route ids
        const auto started = std::chrono::steady_clock::now();
        std::ostringstream table;
        table << "| scene | factor | lateral | served | unfinished | served % | mean delay (s) | changes | waiting at dead end | longest wait (s) | last arrival (s), earliest seed | locked seeds (of the range) | clamps | 2D overlap | removed |\n"
              << "|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|\n";
        std::size_t runs = 0;
        // Rows of one factor and switch, every scene, means over seeds (clamps, overlaps, removals summed).
        for (const auto factor : factors)
            for (const bool lateral : {false, true}) {
                std::vector<Scene> sum(names.size());
                std::vector<double> earliest(names.size(), 1e9);
                std::vector<int> locked(names.size(), 0);
                std::uint64_t clamps = 0, removed = 0, overlaps = 0;
                for (auto seed = first; seed <= last; ++seed, ++runs) {
                    const auto r = sweep::runWeaving(document, data, seed, {factor, lateral, {}, {}});
                    clamps += r.movements.safetyClamps; removed += r.removed; overlaps += r.overlap;
                    for (std::size_t k = 0; k < names.size(); ++k) {
                        const auto s = scene(r, names[k], lab.scenes[k]);
                        earliest[k] = std::min(earliest[k], s.last);
                        // Locked: vehicles left, yet nothing arrived in the last minute of demand.
                        if (s.unfinished > 0 && s.last < kDemandEnd - 60) ++locked[k];
                        sum[k].served += s.served; sum[k].unfinished += s.unfinished; sum[k].delaySum += s.delaySum;
                        sum[k].changes += s.changes; sum[k].waiting += s.waiting;
                        sum[k].longest = std::max(sum[k].longest, s.longest);
                    }
                }
                const auto n = static_cast<double>(last - first + 1);
                for (std::size_t k = 0; k < names.size(); ++k) {
                    const auto& s = sum[k];
                    table << "| " << names[k] << " | " << fixed(factor) << " | " << (lateral ? "on" : "off") << " | "
                          << fixed(s.served / n) << " | " << fixed(s.unfinished / n) << " | "
                          << fixed(100 * s.served / std::max(1.0, s.served + s.unfinished)) << " | "
                          << (s.served > 0 ? fixed(s.delaySum / s.served) : std::string("n/a")) << " | "
                          << fixed(s.changes / n) << " | " << fixed(s.waiting / n) << " | " << fixed(s.longest) << " | " << fixed(earliest[k]) << " | " << locked[k] << " | "
                          << (k == 0 ? std::to_string(clamps) : std::string("")) << " | "
                          << (k == 0 ? std::to_string(overlaps) : std::string("")) << " | "
                          << (k == 0 ? std::to_string(removed) : std::string("")) << " |\n";
                }
            }
        const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count();
        std::cout << "## weaving-lab.traffic.json, seeds " << first << "-" << last << "\n\n"
                  << "Not validated; development evidence (D449). Means per seed; clamps, runs with a 2D overlap and "
                     "removals are totals over the seeds, for all three scenes. " << fixed(seconds) << " s for "
                  << runs << " runs.\n\n" << table.str();
        return std::cout ? 0 : 1;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
