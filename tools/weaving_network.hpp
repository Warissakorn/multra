#pragma once
// The weaving lab (D449, docs/reference/WEAVING_FIXTURE.md): three Type A weaving sections of 150,
// 300 and 500 m, each its own scene, and since D459 a fourth: the 300 m weave with two entries. A development bed and acceptance fixture for the mechanism,
// like the lane-change lab (D98): built through the editor's own commands, committed as
// data/projects/weaving-lab.traffic.json, and `weavinglab` checks the file is this builder's
// output. Regenerate with multra-weaving-fixture, never by hand.
//
// Each scene, left-hand traffic, travel along +x, the kerb side +y: a 3-lane weave Link that starts
// at the network edge (lane 0, the auxiliary lane, kerb side; lanes 1-2 the mainline), then a 1-lane
// exit ramp from lane 0 and a 2-lane mainline from lanes 1-2. The origin is the entry lane: "ramp"
// vehicles enter on lane 0 and "mainline" ones on lanes 1-2, by lane shares. Mainline -> exit and
// ramp -> mainline each cross the lane 0/1 line once, inside the weave.
//
// Why the weave is the entry Link: lane changes compile only on a route's entry Link (M3_8_CONTRACT
// §2, D71/D93), so a weave drawn as a mainline and a ramp Link joining a 3-lane Link cannot carry the
// crossing movements (UNSUPPORTED_ROUTE_TOPOLOGY; `weavinglab` pins it with weavingTwoEntries()).
// Volumes are plausible round numbers, not counts; nothing measured here is a claim about a real
// road (rule 4).
#include "../src/commands/network_commands.hpp"
#include "../src/commands/connector_commands.hpp"
#include "../src/commands/demand_commands.hpp"
#include <array>
#include <stdexcept>
#include <string>

namespace multra::fixture {
struct WeavingScene {
    double length{};
    std::string weave, exitOut, mainOut; // Links
    std::string toMain, toExit;          // routes (scene 4: from the mainline)
    std::string rampMain, rampExit;      // scene 4 only (D459): the routes from the ramp
    std::vector<std::string> routes() const {
        std::vector<std::string> all{toMain, toExit};
        for (const auto& r : {rampMain, rampExit}) if (!r.empty()) all.push_back(r);
        return all;
    }
};
struct WeavingLab {
    ProjectDocument document;
    std::array<WeavingScene, 4> scenes; // three Type A weaves at the edge, then a two-entry one (D459)
};

// Veh/h, one input each: mainline 3000 and ramp 800, 30 % of each crossing
// (volume ratio 0.3). The inputs stop at 600 s and the run continues to 900 s, so the demand drains.
inline constexpr double kMainMain = 2100, kMainExit = 900, kRampMain = 240, kRampExit = 560;

inline WeavingLab weavingLab() {
    constexpr double w = 3.5, gap = 60, joint = 20, away = 300;
    const auto lane = [](const ProjectDocument& d, const std::string& id, std::size_t k) {
        for (const auto& l : d.network.links) if (l.id == id) return l.lanes.at(k).id;
        throw std::invalid_argument("UNKNOWN_LINK");
    };
    WeavingLab t;
    auto& d = t.document;
    d.network.id = "weaving-lab";
    d.network.drivingSide = DrivingSide::left;
    d.definition = AuthoringDefinition{};
    const std::array<double, 3> lengths{150, 300, 500};
    for (std::size_t k = 0; k < lengths.size(); ++k) {
        auto& sc = t.scenes[k];
        sc.length = lengths[k];
        const double y = -gap * static_cast<double>(k), x1 = sc.length;
        const std::string name = "Weave " + std::to_string(static_cast<int>(sc.length)) + " m";
        // The weave's lanes lie at y + w (lane 0), y and y - w; the 2-lane mainline matches lanes 1-2,
        // and the 1-lane exit lies one lane further out, so the two Connectors do not cross.
        sc.weave = addLink(d, {{0, y}, {x1, y}}, 3, w);
        sc.exitOut = addLink(d, {{x1 + joint, y + 2 * w}, {x1 + joint + away, y + 2 * w}}, 1, w);
        sc.mainOut = addLink(d, {{x1 + joint, y - w / 2}, {x1 + joint + away, y - w / 2}}, 2, w);
        editableLink(d, sc.weave).name = name;
        editableLink(d, sc.exitOut).name = name + " exit";
        editableLink(d, sc.mainOut).name = name + " mainline";
        const auto exitOff = addConnector(d, {sc.weave, lane(d, sc.weave, 0)}, {sc.exitOut, lane(d, sc.exitOut, 0)});
        const auto mainOff = addConnectorRange(d, {sc.weave, lane(d, sc.weave, 1)}, {sc.mainOut, lane(d, sc.mainOut, 0)}, 2, 2);
        editableConnector(d, exitOff).name = name + " to exit";
        editableConnector(d, mainOff).name = name + " to mainline";
        sc.toMain = putRoute(d, {"", {sc.weave, mainOff, sc.mainOut}});
        sc.toExit = putRoute(d, {"", {sc.weave, exitOff, sc.exitOut}});
        const std::vector<double> ramp{1, 0, 0}, mainline{0, 1, 1}; // by the weave's lane order
        putInput(d, {"", sc.toMain, "car", kMainMain, 0, 600, mainline});
        putInput(d, {"", sc.toExit, "car", kMainExit, 0, 600, mainline});
        putInput(d, {"", sc.toMain, "car", kRampMain, 0, 600, ramp});
        putInput(d, {"", sc.toExit, "car", kRampExit, 0, 600, ramp});
    }
    // D459, scene 4: the 300 m weave drawn with two entries -- a 2-lane mainline and a 1-lane ramp,
    // each with a 300 m approach, joining the 3-lane weave -- as weavingTwoEntries() draws it. The
    // crossing movements change onto joined chains that start on the weave.
    {
        auto& sc = t.scenes[3];
        sc.length = 300;
        const double y = -3 * gap, x0 = 300 + joint, x1 = x0 + sc.length;
        const std::string name = "Two-entry 300 m";
        const auto mainIn = addLink(d, {{0, y - w / 2}, {300, y - w / 2}}, 2, w);
        const auto rampIn = addLink(d, {{0, y + 2 * w}, {300, y + 2 * w}}, 1, w);
        sc.weave = addLink(d, {{x0, y}, {x1, y}}, 3, w);
        sc.exitOut = addLink(d, {{x1 + joint, y + 2 * w}, {x1 + joint + away, y + 2 * w}}, 1, w);
        sc.mainOut = addLink(d, {{x1 + joint, y - w / 2}, {x1 + joint + away, y - w / 2}}, 2, w);
        editableLink(d, mainIn).name = name + " mainline approach";
        editableLink(d, rampIn).name = name + " ramp";
        editableLink(d, sc.weave).name = name + " weave";
        editableLink(d, sc.exitOut).name = name + " exit";
        editableLink(d, sc.mainOut).name = name + " mainline";
        const auto mainOn = addConnectorRange(d, {mainIn, lane(d, mainIn, 0)}, {sc.weave, lane(d, sc.weave, 1)}, 2, 2);
        const auto rampOn = addConnector(d, {rampIn, lane(d, rampIn, 0)}, {sc.weave, lane(d, sc.weave, 0)});
        const auto exitOff = addConnector(d, {sc.weave, lane(d, sc.weave, 0)}, {sc.exitOut, lane(d, sc.exitOut, 0)});
        const auto mainOff = addConnectorRange(d, {sc.weave, lane(d, sc.weave, 1)}, {sc.mainOut, lane(d, sc.mainOut, 0)}, 2, 2);
        editableConnector(d, mainOn).name = name + " mainline on";
        editableConnector(d, rampOn).name = name + " ramp on";
        editableConnector(d, exitOff).name = name + " to exit";
        editableConnector(d, mainOff).name = name + " to mainline";
        sc.toMain = putRoute(d, {"", {mainIn, mainOn, sc.weave, mainOff, sc.mainOut}});
        sc.toExit = putRoute(d, {"", {mainIn, mainOn, sc.weave, exitOff, sc.exitOut}});
        sc.rampMain = putRoute(d, {"", {rampIn, rampOn, sc.weave, mainOff, sc.mainOut}});
        sc.rampExit = putRoute(d, {"", {rampIn, rampOn, sc.weave, exitOff, sc.exitOut}});
        putInput(d, {"", sc.toMain, "car", kMainMain, 0, 600, {}});
        putInput(d, {"", sc.toExit, "car", kMainExit, 0, 600, {}});
        putInput(d, {"", sc.rampMain, "car", kRampMain, 0, 600, {}});
        putInput(d, {"", sc.rampExit, "car", kRampExit, 0, 600, {}});
    }
    changeRunSettings(d, 900, 0.1);
    return t;
}
// The same weave drawn with two entries -- a 2-lane mainline and a 1-lane ramp Link joining the
// 3-lane weave -- and its four routes, without inputs. Before D459 it did not compile: the
// crossing routes had no single-lane path. BA179 now pins that it does (joined chains).
inline ProjectDocument weavingTwoEntries() {
    constexpr double w = 3.5, length = 300;
    const auto lane = [](const ProjectDocument& d, const std::string& id, std::size_t k) {
        for (const auto& l : d.network.links) if (l.id == id) return l.lanes.at(k).id;
        throw std::invalid_argument("UNKNOWN_LINK");
    };
    ProjectDocument d;
    d.network.drivingSide = DrivingSide::left;
    d.definition = AuthoringDefinition{};
    const auto mainIn = addLink(d, {{0, -w / 2}, {300, -w / 2}}, 2, w);
    const auto rampIn = addLink(d, {{0, 2 * w}, {300, 2 * w}}, 1, w);
    const auto weave = addLink(d, {{320, 0}, {320 + length, 0}}, 3, w);
    const auto exitOut = addLink(d, {{340 + length, 2 * w}, {640 + length, 2 * w}}, 1, w);
    const auto mainOut = addLink(d, {{340 + length, -w / 2}, {640 + length, -w / 2}}, 2, w);
    const auto mainOn = addConnectorRange(d, {mainIn, lane(d, mainIn, 0)}, {weave, lane(d, weave, 1)}, 2, 2);
    const auto rampOn = addConnector(d, {rampIn, lane(d, rampIn, 0)}, {weave, lane(d, weave, 0)});
    const auto exitOff = addConnector(d, {weave, lane(d, weave, 0)}, {exitOut, lane(d, exitOut, 0)});
    const auto mainOff = addConnectorRange(d, {weave, lane(d, weave, 1)}, {mainOut, lane(d, mainOut, 0)}, 2, 2);
    putRoute(d, {"main-main", {mainIn, mainOn, weave, mainOff, mainOut}});
    putRoute(d, {"main-exit", {mainIn, mainOn, weave, exitOff, exitOut}});
    putRoute(d, {"ramp-main", {rampIn, rampOn, weave, mainOff, mainOut}});
    putRoute(d, {"ramp-exit", {rampIn, rampOn, weave, exitOff, exitOut}});
    changeRunSettings(d, 900, 0.1);
    return d;
}
}
