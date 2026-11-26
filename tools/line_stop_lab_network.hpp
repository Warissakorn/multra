#pragma once
// LINE_SETBACK §8 (D671): the line-stop lab, data/projects/line-stop-lab.traffic.json, for the owner's Windows
// checks of stopping at lines, queues at rest and lineSetback without typing model keys. Not a study: the four-leg
// (fixture::shippedFourLeg) with its catalogs owned and three more behaviours.
//   - w74-lab: the BA27 W74 fixture (docs/evidence/w74-discharge-behaviour.json);
//   - w74-sumo: the SUMO-defaults preset (data/driver-behaviour/presets/w74-sumo-defaults.json, D176);
//   - w99-lab: the BA231 W99 fixture (docs/evidence/w99-discharge-behaviour.json, W99 §5).
// Every vehicle type starts on w99-lab; switch a type's behaviour in the behaviour library to compare. The fixture
// values are test values, not presets, and nothing here is validated (rule 4). Regenerate with
// multra-line-stop-lab-fixture, never by hand; `linesetback` checks the file is this output.
#include "four_leg_network.hpp"
#include "../src/commands/behaviour_commands.hpp"
#include "../src/commands/catalog_commands.hpp"
#include "../src/project/demand_catalog.hpp"
#include <filesystem>

namespace multra::fixture {
inline ProjectDocument lineStopLab(const std::filesystem::path& data) {
    auto d = shippedFourLeg();
    d.network.id = "line-stop-lab";
    putDemandCatalog(d, resolveDemandCatalog(*d.definition, data));
    DriverBehaviour w74Lab{"w74-lab"};
    w74Lab.w74 = W74Parameters{2, 2, 3, 1.5, 1, 25, 15, 1.5, 0.5, 150, 1.5, 0.5, 4, 0.2, 0.2, -1, 0.5, 0.5};
    putBehaviour(d, w74Lab, "W74 test (BA27 fixture, not a preset)");
    DriverBehaviour w74Sumo{"w74-sumo"};
    w74Sumo.w74 = W74Parameters{2, 4.5, 0, 1.5, 0, 50, 0, 1, 2, 150, 0.2, 2.08, 7, 0.52, 0, -1, 0.5, 0.55};
    putBehaviour(d, w74Sumo, "W74 SUMO defaults (preset, uncalibrated)");
    DriverBehaviour w99Lab{"w99-lab"};
    w99Lab.w99 = W99Parameters{1.5, 1.3, 4, -12, -0.25, 0.35, 6, 0.25, 2, 1.5, {}};
    putBehaviour(d, w99Lab, "W99 test (BA231 fixture, not a preset)");
    for (auto& t : d.definition->vehicleTypes) t.behaviourId = "w99-lab";
    validateDocument(d);
    return d;
}
}
