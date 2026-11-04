#pragma once
// The lateral lab (D457, docs/reference/LATERAL_LAB.md): four small scenes for watching lateral
// behaviour with the switch on -- the one shipped project whose behaviours carry the lateral keys.
// A development bed, not a study and not an acceptance fixture for any gate, like the lane-change
// lab (D98) and the weaving lab (D449): built through the editor's own commands, committed as
// data/projects/lateral-lab.traffic.json, and `laterallab` checks the file is this builder's output.
// Regenerate with multra-lateral-fixture, never by hand.
//
// The scenes share nothing and lie 60 m apart, so each is its own movement row. Left-hand traffic,
// travel along +x, lane 0 the kerb lane (+y):
//   Timed lane change  the lane-change lab's diverge: 3 lanes splitting into a 1-lane exit (lane 0)
//                      and a 2-lane exit (lanes 1-2); each change takes time (D204) and straddles.
//   Lane drop          the lane-change lab's lane drop: 2 lanes, a 1-lane Connector, 2 lanes.
//   Short weave        the weaving lab's 150 m Type A weave at x0.8 of its volumes.
//   Keep-left          one 2-lane Link, cars, heavy vehicles and motorcycles; its own behaviour type
//                      carries D95's pair and the kerb-lane bias (D215).
// The geometry is copied, not shared, so the other two labs keep their bytes. Every key value here
// is a lab placeholder: uncited, not a preset, not yet validated (rule 4; D178 waived for the lab).
// Volumes are plausible round numbers, not counts.
#include "lateral_lab_keys.hpp"
#include "weaving_network.hpp"
#include "../src/commands/behaviour_commands.hpp"
#include "../src/commands/catalog_commands.hpp"
#include <filesystem>

namespace multra::fixture {
struct LateralLab {
    ProjectDocument document;
    std::string exitARoute, exitBRoute;   // scene 1
    std::string dropRoute;                // scene 2
    std::string weaveMain, weaveExit;     // scene 3
    std::string keepLeftRoute, keepLeft;  // scene 4: the route and its Link
};
inline constexpr auto kKeepLeftBehaviour = "lateral-lab-keep-left";
inline constexpr auto kKeepLeftType = "lateral-lab-keep-left";

// `data` is the installed data directory: the behaviour and vehicle-type catalogs are captured
// from it, so the project owns them and carries the keys.
inline LateralLab lateralLab(const std::filesystem::path& data) {
    constexpr double w = 3.5, gap = 60;
    const auto lane = [](const ProjectDocument& d, const std::string& id, std::size_t k) {
        for (const auto& l : d.network.links) if (l.id == id) return l.lanes.at(k).id;
        throw std::invalid_argument("UNKNOWN_LINK");
    };
    LateralLab t;
    auto& d = t.document;
    d.network.id = "lateral-lab";
    d.network.drivingSide = DrivingSide::left;
    d.definition = AuthoringDefinition{};

    // 1. Timed lane change (the lane-change lab's diverge).
    {
        const auto diverge = addLink(d, {{0, 0}, {600, 0}}, 3, w);
        const auto exitA = addLink(d, {{620, 2 * w}, {900, 2 * w}}, 1, w);
        const auto exitB = addLink(d, {{620, -w / 2}, {900, -w / 2}}, 2, w);
        editableLink(d, diverge).name = "Timed lane change";
        editableLink(d, exitA).name = "Timed lane change exit A";
        editableLink(d, exitB).name = "Timed lane change exit B";
        const auto toA = addConnector(d, {diverge, lane(d, diverge, 0)}, {exitA, lane(d, exitA, 0)});
        const auto toB = addConnectorRange(d, {diverge, lane(d, diverge, 1)}, {exitB, lane(d, exitB, 0)}, 2, 2);
        editableConnector(d, toA).name = "To exit A";
        editableConnector(d, toB).name = "To exit B";
        t.exitARoute = putRoute(d, {"", {diverge, toA, exitA}});
        t.exitBRoute = putRoute(d, {"", {diverge, toB, exitB}});
        putInput(d, {"", t.exitARoute, "car", 400, 0, 600, {}});
        putInput(d, {"", t.exitBRoute, "car", 1000, 0, 600, {}});
        putInput(d, {"", t.exitBRoute, "heavy-vehicle", 150, 0, 600, {}});
    }
    // 2. Lane drop.
    {
        const double y = -gap;
        const auto up = addLink(d, {{0, y}, {300, y}}, 2, w);
        const auto down = addLink(d, {{320, y}, {820, y}}, 2, w);
        editableLink(d, up).name = "Lane drop upstream";
        editableLink(d, down).name = "Lane drop downstream";
        const auto drop = addConnector(d, {up, lane(d, up, 0)}, {down, lane(d, down, 0)});
        editableConnector(d, drop).name = "Lane drop";
        t.dropRoute = putRoute(d, {"", {up, drop, down}});
        putInput(d, {"", t.dropRoute, "car", 900, 0, 600, {}});
    }
    // 3. Short weave (the weaving lab's 150 m scene, x0.8).
    {
        constexpr double length = 150, joint = 20, away = 300, factor = 0.8;
        const double y = -2 * gap;
        const auto weave = addLink(d, {{0, y}, {length, y}}, 3, w);
        const auto exitOut = addLink(d, {{length + joint, y + 2 * w}, {length + joint + away, y + 2 * w}}, 1, w);
        const auto mainOut = addLink(d, {{length + joint, y - w / 2}, {length + joint + away, y - w / 2}}, 2, w);
        editableLink(d, weave).name = "Short weave";
        editableLink(d, exitOut).name = "Short weave exit";
        editableLink(d, mainOut).name = "Short weave mainline";
        const auto exitOff = addConnector(d, {weave, lane(d, weave, 0)}, {exitOut, lane(d, exitOut, 0)});
        const auto mainOff = addConnectorRange(d, {weave, lane(d, weave, 1)}, {mainOut, lane(d, mainOut, 0)}, 2, 2);
        editableConnector(d, exitOff).name = "Short weave to exit";
        editableConnector(d, mainOff).name = "Short weave to mainline";
        t.weaveMain = putRoute(d, {"", {weave, mainOff, mainOut}});
        t.weaveExit = putRoute(d, {"", {weave, exitOff, exitOut}});
        const std::vector<double> ramp{1, 0, 0}, mainline{0, 1, 1}; // by the weave's lane order
        putInput(d, {"", t.weaveMain, "car", factor * kMainMain, 0, 600, mainline});
        putInput(d, {"", t.weaveExit, "car", factor * kMainExit, 0, 600, mainline});
        putInput(d, {"", t.weaveMain, "car", factor * kRampMain, 0, 600, ramp});
        putInput(d, {"", t.weaveExit, "car", factor * kRampExit, 0, 600, ramp});
    }
    // 4. Keep-left with a mixed fleet.
    {
        const double y = -3 * gap;
        t.keepLeft = addLink(d, {{0, y}, {1000, y}}, 2, w);
        editableLink(d, t.keepLeft).name = "Keep-left";
        t.keepLeftRoute = putRoute(d, {"", {t.keepLeft}});
        putInput(d, {"", t.keepLeftRoute, "car", 900, 0, 600, {}});
        putInput(d, {"", t.keepLeftRoute, "heavy-vehicle", 200, 0, 600, {}});
        putInput(d, {"", t.keepLeftRoute, "motorcycle", 150, 0, 600, {}});
    }
    changeRunSettings(d, 900, 0.1);

    // Behaviours: the project owns the installed catalogs, every behaviour carries the lab keys, and
    // the keep-left Link runs a copy that also carries D95's pair and the bias (kerb_bias_tests' values).
    auto catalog = resolveDemandCatalog(*d.definition, data);
    for (auto& b : catalog.behaviours) b.lateral = labLateralKeys();
    const auto base = catalog.behaviours.front();
    putDemandCatalog(d, std::move(catalog));
    auto keep = base;
    keep.id = kKeepLeftBehaviour;
    keep.discretionaryLaneChangeThreshold = 0.5; keep.acceptedDecelerationTrailingVehicle = 1; keep.kerbLaneBias = 1;
    putBehaviour(d, keep, "Lateral lab keep-left");
    putLinkBehaviourType(d, {kKeepLeftType, "Keep-left", kKeepLeftBehaviour, {}});
    assignBehaviourType(d, t.keepLeft, kKeepLeftType);
    return t;
}
}
