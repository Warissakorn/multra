#pragma once
// A counted four-leg study built on the M2.6 drawing (tools/m26_study_network.hpp's geometry): the
// four-leg with right-turn pockets and the Thai left turn at all times, fed by a count sheet and a
// fixed-time plan read from two CSV files, so the counts stay data and are typed once (D142).
//
//   counts.csv  the Sheet's count tab exported as CSV, one row per approach x 15-min interval x
//               movement: leg, interval (1-4), time label, movement, motorcycle, car, bus, truck[, total].
//               Movement is left / through / right / U-turn (Thai labels ซ้าย ตรง ขวา กลับรถ). The
//               drawing has no U-turn exit, so a U-turn count joins the right turn (same lane, same
//               phase): approach totals stay as counted, the destination does not.
//   site.csv    `cycle,<s>`, one `leg,<leg>,<West|East|South|North>,<green start>,<green end>` per
//               approach (the green window in seconds of the cycle; amber 3 s follows), and
//               optionally `lanes,<leg>,<stop-line lanes>,<exit lanes>[,<right-turn lanes>]` per leg and `stopline,<m>`
//               (metres from the centre, clear of the widest crossing leg). Missing: the four-leg's.
//               `entrylanes,serving` puts each movement on the entry lanes that serve it (D580,
//               LANE_LOCKS option A); absent, every movement spreads over every lane (M3.2.8b).
//
// Each approach gets one routeless input whose volume IS its routing decision's counts
// (volumeFromCounts), and its own composition from its counted class shares. The four counted
// classes map to the installed motorcycle, car, bus and rigid-truck types. The project owns its
// demand catalog so it opens with no data directory. Nothing here is validated (rule 4).
#include "four_leg_network.hpp"
#include "../src/commands/catalog_commands.hpp"
#include "../src/project/demand_catalog.hpp"
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>

namespace multra::fixture {
namespace count_study {
inline std::vector<std::vector<std::string>> readCsv(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::invalid_argument("COUNT_STUDY_UNREADABLE: " + path.string());
    std::vector<std::vector<std::string>> rows;
    std::string line;
    while (std::getline(in, line)) {
        if (rows.empty() && line.rfind("\xEF\xBB\xBF", 0) == 0) line.erase(0, 3);  // UTF-8 BOM
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line[0] == '#') continue;
        std::vector<std::string> cells;
        std::stringstream s(line);
        for (std::string cell; std::getline(s, cell, ',');) cells.push_back(cell);
        rows.push_back(cells);
    }
    return rows;
}
// 0 through, 1 left, 2 right: the four-leg's movement order. A U-turn joins the right turn.
inline std::size_t movementIndex(const std::string& m) {
    if (m == "through" || m == "ตรง") return 0;
    if (m == "left" || m == "ซ้าย") return 1;
    if (m == "right" || m == "ขวา" || m == "U-turn" || m == "กลับรถ") return 2;
    throw std::invalid_argument("COUNT_STUDY_UNKNOWN_MOVEMENT: " + m);
}
}

inline constexpr std::array<const char*, 4> kCountedTypes{"motorcycle", "car", "bus", "rigid-truck"};

inline ProjectDocument countStudy(const std::filesystem::path& data, const std::filesystem::path& countsCsv,
                                  const std::filesystem::path& siteCsv, const std::string& id) {
    using namespace count_study;
    const std::array<const char*, 4> approaches{"West", "East", "South", "North"};
    const auto approachIndex = [&](const std::string& name) {
        for (std::size_t k = 0; k < 4; ++k) if (name == approaches[k]) return k;
        throw std::invalid_argument("COUNT_STUDY_UNKNOWN_APPROACH: " + name);
    };
    const auto site = readCsv(siteCsv);
    std::map<std::string, std::size_t> legOf;  // the site's leg name -> four-leg approach
    std::array<std::string, 4> legName;
    for (const auto& row : site)
        if (row.at(0) == "leg") { const auto k = approachIndex(row.at(2)); legOf[row.at(1)] = k; legName[k] = row.at(1); }
    if (legOf.size() != 4) throw std::invalid_argument("COUNT_STUDY_NEEDS_FOUR_LEGS");
    bool serving = false;
    for (const auto& row : site) if (row.at(0) == "entrylanes") serving = row.at(1) == "serving";
    FourLegOptions options;
    options.leftBypass = 6;  // the Thai left turn at all times, as in the M2.6 template
    for (const auto& row : site) {
        if (row.at(0) == "stopline") options.stopLine = std::stod(row.at(1));
        if (row.at(0) != "lanes") continue;
        const auto k = legOf.at(row.at(1));
        options.approachLanes[k] = std::stoi(row.at(2)); options.exitLanes[k] = std::stoi(row.at(3));
        if (row.size() > 4) options.rightLanes[k] = std::stoi(row[4]);
        if (options.approachLanes[k] < 2 || options.exitLanes[k] < 1) throw std::invalid_argument("COUNT_STUDY_LANES");
    }
    auto built = fourLegIntersection(options);
    auto& d = built.document;
    d.network.id = id;
    for (const auto& route : built.routes) deleteRoute(d, route);  // cascades the fixture inputs

    auto controller = demand(d).signalControllers.at(0);
    controller.name = id + " fixed-time plan";
    for (const auto& row : site) {
        if (row.at(0) == "cycle") controller.cycle = std::stod(row.at(1));
        if (row.at(0) != "leg") continue;
        auto& group = controller.groups[legOf.at(row.at(1))];
        group.name = row.at(1);
        group.greenStart = std::stod(row.at(3));
        group.greenEnd = std::stod(row.at(4));
    }
    putSignalController(d, controller);

    // counts[approach][movement][interval], vehicles; classes[approach][type], vehicles.
    std::array<std::array<std::array<double, 4>, 3>, 4> counts{};
    std::array<std::array<double, 4>, 4> classes{};
    for (const auto& row : readCsv(countsCsv)) {
        if (!legOf.count(row.at(0))) continue;  // the header, or a row of another site
        const auto k = legOf.at(row[0]);
        const auto interval = std::stoul(row.at(1)) - 1;
        if (interval >= 4) throw std::invalid_argument("COUNT_STUDY_INTERVAL");
        double total = 0;
        for (std::size_t c = 0; c < 4; ++c) {
            const double n = row.at(4 + c).empty() ? 0 : std::stod(row[4 + c]);
            classes[k][c] += n; total += n;
        }
        counts[k][movementIndex(row.at(3))][interval] += total;
    }

    auto catalog = resolveDemandCatalog(*d.definition, data);
    for (std::size_t k = 0; k < 4; ++k) {
        Composition c{id + "-" + approaches[k], {}, legName[k] + " (counted)", {}};
        for (std::size_t t = 0; t < 4; ++t)
            if (classes[k][t] > 0) c.types.push_back({kCountedTypes[t], classes[k][t]});
        catalog.compositions.push_back(c);
    }
    putDemandCatalog(d, std::move(catalog));

    const auto named = [&](const std::string& name) {
        for (const auto& l : d.network.links) if (l.name == name) return l.id;
        throw std::invalid_argument("UNKNOWN_LINK");
    };
    const std::array<std::size_t, 4> through{1, 0, 3, 2}, left{3, 2, 0, 1}, right{2, 3, 1, 0};
    constexpr double period = 900;
    std::vector<DecisionInterval> slots;
    for (int i = 0; i < 4; ++i) slots.push_back({i * period, (i + 1) * period});
    for (std::size_t k = 0; k < 4; ++k) {
        const std::string name = approaches[k];
        const auto entry = named(name + " approach");
        RoutingDecision decision{"", legName[k] + " turning counts", {}, entry, slots};
        if (serving) decision.entryLanes = "serving";
        const std::array<std::size_t, 3> to{through[k], left[k], right[k]};
        for (std::size_t m = 0; m < 3; ++m) {
            const auto& c = counts[k][m];
            const double hour = c[0] + c[1] + c[2] + c[3];
            if (hour <= 0) continue;
            decision.routes.push_back({"", hour, named(std::string(approaches[to[m]]) + " exit"),
                                       std::vector<double>(c.begin(), c.end())});
        }
        putRoutingDecision(d, decision);
        VehicleInput input{"", "", "car", 0, 0, 4 * period, {}};
        input.compositionId = id + "-" + name;
        input.linkId = entry;
        input.volumeFromCounts = true;
        putInput(d, input);
    }
    changeRunSettings(d, 4 * period, 0.1);
    return d;
}
}
