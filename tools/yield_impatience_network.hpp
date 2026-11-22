#pragma once
// YIELD_IMPATIENCE (D618): the waiting-acceptance lab, data/projects/yield-impatience-lab.traffic.json,
// for the owner's Windows look. Not a study: the T-junction (fixture::tJunction, right-hand, Yield at
// its lines) with a busier major road, every area's rule reducing its gap as Aimsun does, and every
// behaviour of the owned catalog carrying a yield time and SUMO's impatience. Each value is a lab
// placeholder: uncited, not a preset, not yet validated (rule 4). Regenerate with
// multra-yield-impatience-fixture, never by hand; `yield_impatience` checks the file is this output.
//   - gap: 5 s down to 2 s, from 1 × to 2 × the driver's yield time (Aimsun's GW Time Factors);
//   - yield time: 20 s mean, 5 s deviation, within 5-40 s, drawn per vehicle;
//   - impatience 0.2 (SUMO's base), full after 180 s: a standing major vehicle lets the next minor
//     vehicle follow the first.
#include "t_junction_network.hpp"
#include "../src/commands/catalog_commands.hpp"
#include <filesystem>

namespace multra::fixture {
struct YieldImpatienceLab { ProjectDocument document; TJunction roads; };
inline void yieldLabKeys(DriverBehaviour& b) {
    b.yieldTime = 20; b.yieldTimeDeviation = 5; b.yieldTimeMinimum = 5; b.yieldTimeMaximum = 40; b.impatience = 0.2;
}
inline YieldImpatienceLab yieldImpatienceLab(const std::filesystem::path& data) {
    YieldImpatienceLab lab;
    lab.roads = tJunction();
    auto& d = lab.document = lab.roads.document;
    d.network.id = "yield-impatience-lab";
    for (auto& input : d.definition->inputs)
        if (input.routeId == lab.roads.eastRoute || input.routeId == lab.roads.westRoute) input.vehiclesPerHour = 700;
    for (auto& r : d.network.rightOfWay.priorityRules) r.gapReduction = GapReduction{2, 1, 2};
    auto catalog = resolveDemandCatalog(*d.definition, data);
    for (auto& b : catalog.behaviours) yieldLabKeys(b);
    putDemandCatalog(d, std::move(catalog));
    validateDocument(d);
    return lab;
}
}
