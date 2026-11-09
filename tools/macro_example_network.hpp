#pragma once
// The Macro example (data/projects/macro-four-step.traffic.json): a small town drawn for the Macro
// level alone, with every four-step input the project can own, so `--procedure four-step` runs trip
// generation -> distribution -> mode choice -> assignment with no catalog outside the file except the
// installed VDF and modes. Built through the editor's own commands; `macroexample` fails
// until the committed file is this builder's output. Regenerate with multra-macro-example, never by hand.
//
// Layout, left-hand traffic, metres: a centre node X at (0, 0) and four outer nodes W, N, E, S at
// 5 km. Two-way radial arterials (2 lanes) join each outer node to X; a two-way ring road (2 lanes,
// faster) joins the outer nodes W-N-E-S-W. The project owns its three link types at *practical*
// capacities, the basis BPR was fitted to: the installed HCM types are ultimate capacities, which
// Macro refuses to pair with `bpr` (MACRO_CAPACITY_BASIS_MISMATCH, D304). Each outer node has a zone beyond it,
// reached by its own access Link pair, so the origin's split over the three roads leaving the node is
// route choice, not a fixed zone-connector share (MACRO_NETWORK §3). Connectors join every arriving
// Link to every leaving one except its own reverse (no U-turns). W->E is 10 km on the arterials (60 km/h) and
// 14.1 km on the ring (80 km/h): at free flow the arterials are quicker by about 6 %, and the AM peak
// loads them until the two routes cost the same, so part of the flow moves to the ring.
//
// Every number here is an illustration chosen to show the method, not a count, a survey or a
// calibrated value; no output is a claim about a real town (rule 4) and every result keeps the
// not-validated marker.
#include "../src/commands/catalog_commands.hpp"
#include "../src/commands/connector_commands.hpp"
#include "../src/commands/demand_commands.hpp"
#include "../src/commands/mode_choice_commands.hpp"
#include "../src/commands/network_commands.hpp"
#include "../src/commands/procedure_commands.hpp"
#include "../src/commands/trip_rate_commands.hpp"
#include "../src/commands/zone_commands.hpp"
#include "../src/project/cost_matrix_codec.hpp"
#include "../src/project/demand_catalog.hpp"
#include "../src/project/period_factor_codec.hpp"
#include "../src/project/procedure_codec.hpp"
#include <array>
#include <cmath>
#include <filesystem>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace multra::fixture {
inline constexpr const char* kMacroExampleCitation =
    "Illustrative values for the shipped Macro example only (data/projects/README.md); not calibrated, not a study";

struct MacroExampleZone {
    const char* node;     // the outer node it hangs off
    const char* id;
    const char* name;
    double households, jobs, retailJobs;
};
// A dormitory west and north, a mixed south, and the employment centre east.
inline constexpr std::array<MacroExampleZone, 4> kMacroExampleZones{{
    {"W", "west", "West (housing)", 28000, 6000, 2000},
    {"N", "north", "North (housing)", 21000, 5000, 1500},
    {"E", "east", "East (centre of employment)", 3500, 60000, 5000},
    {"S", "south", "South (mixed, retail)", 14000, 15000, 6000},
}};

struct MacroExample {
    ProjectDocument document;
    std::map<std::string, std::string> links; // "W-X", "X-E", "in:W", "out:W", ... -> Link id
};

// The catalogs the project owns, in their stored JSON form (TRIP_GENERATION §6.3, TRIP_DISTRIBUTION §5-§6,
// MODE_CHOICE §5-§6, PROCEDURE_SEQUENCE §2), parsed by the same readers a saved file goes through.
inline Json macroExampleCatalogs() {
    const Json source = {{"citation", kMacroExampleCitation}};
    // Zone order of every matrix below; the transit minutes are symmetric, the diagonal unused.
    const std::vector<std::string> zones{"west", "north", "east", "south"};
    const auto transit = Json::array({
        {nullptr, 30, 25, 30},
        {30, nullptr, 22, 28},
        {25, 22, nullptr, 20},
        {30, 28, 20, nullptr},
    });
    const auto alternatives = [](double transitConstant) {
        return Json::array({
            {{"id", "car"}, {"modeId", "car"}, {"constant", 0}, {"timeCoefficient", -0.10}, {"occupancy", 1.2}},
            {{"id", "transit"}, {"costMatrix", "transit-times"}, {"constant", transitConstant}, {"timeCoefficient", -0.05}},
        });
    };
    return {
        {"tripRateSets", Json::array({{
            {"id", "town-daily"}, {"name", "Town example, daily person trips"}, {"source", source}, {"unit", "person trips per day"},
            {"purposes", Json::array({
                {{"id", "hbw"},
                 {"productions", {{"method", "crossClassification"}, {"categories", Json::array({{{"attribute", "households"}, {"rate", 1.5}}})}}},
                 {"attractions", {{"method", "regression"}, {"constant", 0}, {"terms", Json::array({{{"attribute", "jobs"}, {"coefficient", 1}}})}}}},
                {{"id", "hbo"},
                 {"productions", {{"method", "crossClassification"}, {"categories", Json::array({{{"attribute", "households"}, {"rate", 2.0}}})}}},
                 {"attractions", {{"method", "regression"}, {"constant", 0},
                                  {"terms", Json::array({{{"attribute", "households"}, {"coefficient", 0.6}},
                                                         {{"attribute", "retailJobs"}, {"coefficient", 4}}})}}}},
            })},
        }})},
        {"deterrenceSets", Json::array({{
            {"id", "town-gravity"}, {"name", "Town example, exponential gravity"}, {"source", source},
            {"function", "exponential"}, {"costUnit", "min"},
            {"purposes", Json::array({{{"id", "hbw"}, {"beta", 0.08}}, {{"id", "hbo"}, {"beta", 0.15}}})},
        }})},
        {"costMatrices", Json::array({{
            {"id", "transit-times"}, {"name", "Transit door-to-door minutes (illustrative)"}, {"unit", "min"},
            {"zoneIds", zones}, {"rows", transit},
        }})},
        {"modeChoiceSets", Json::array({{
            {"id", "town-logit"}, {"name", "Town example, car and transit logit"}, {"source", source}, {"costUnit", "min"},
            {"purposes", Json::array({{{"id", "hbw"}, {"alternatives", alternatives(-0.8)}},
                                      {{"id", "hbo"}, {"alternatives", alternatives(-1.5)}}})},
        }})},
        // The AM peak hour's share of each purpose's daily trips, by direction.
        {"periodFactors", Json::array({
            {{"purposeId", "hbw"}, {"periodId", "period-am"}, {"productionToAttraction", 0.10}, {"attractionToProduction", 0.01}},
            {{"purposeId", "hbo"}, {"periodId", "period-am"}, {"productionToAttraction", 0.03}, {"attractionToProduction", 0.015}},
        })},
        {"procedures", Json::array({{
            {"id", "four-step"}, {"name", "Four steps, AM peak"},
            {"steps", Json::array({
                {{"kind", "tripGeneration"}, {"rateSet", "town-daily"}},
                {{"kind", "tripDistribution"}, {"deterrenceSet", "town-gravity"}, {"skimMode", "car"}},
                {{"kind", "modeChoice"}, {"modeChoiceSet", "town-logit"}, {"periods", Json::array({"period-am"})}},
                {{"kind", "assignment"}, {"period", "period-am"}},
            })},
        }})},
    };
}

// `data` is the installed data directory: the project owns its demand catalog (vehicle types, compositions,
// behaviours) so the car can carry the PCU an assignment needs (MODE_PCU_MISSING otherwise).
inline MacroExample macroExample(const std::filesystem::path& data) {
    constexpr double reach = 5000, pullBack = 40, side = 5, access = 600, w = 3.5;
    const std::map<std::string, Point> node{{"X", {0, 0}}, {"W", {-reach, 0}}, {"N", {0, reach}}, {"E", {reach, 0}}, {"S", {0, -reach}}};
    const auto lane = [](const ProjectDocument& d, const std::string& id, std::size_t k) {
        for (const auto& l : d.network.links) if (l.id == id) return l.lanes.at(k).id;
        throw std::invalid_argument("UNKNOWN_LINK");
    };
    MacroExample t;
    auto& d = t.document;
    d.network.id = "macro-four-step";
    d.network.drivingSide = DrivingSide::left;
    d.definition = AuthoringDefinition{};
    // Practical capacities (pcu/h per lane) and free speeds (m/s), illustrations like every number here.
    for (const auto& type : {LinkType{"arterial", "Urban arterial (example)", 900, 60 / 3.6, "bpr", {}, kMacroExampleCitation},
                             LinkType{"ring-road", "Ring road (example)", 1600, 80 / 3.6, "bpr", {}, kMacroExampleCitation},
                             LinkType{"access", "Zone access (example)", 3600, 50 / 3.6, "bpr", {}, kMacroExampleCitation}})
        putLinkType(d, type, {}); // nothing installed is owned with them: the project needs only these three
    // A two-way road is two Links, each drawn on its own (left-hand) side and pulled back from both
    // node centres so the Connectors have room to turn.
    const auto draw = [&](Point a, Point b, const std::string& key, const std::string& name, const char* type) {
        const double dx = b.x - a.x, dy = b.y - a.y, len = std::hypot(dx, dy);
        const double ux = dx / len, uy = dy / len, nx = -uy * side, ny = ux * side;
        const auto id = addLink(d, {{a.x + ux * pullBack + nx, a.y + uy * pullBack + ny}, {b.x - ux * pullBack + nx, b.y - uy * pullBack + ny}}, 2, w);
        editableLink(d, id).name = name;
        setLinkType(d, id, std::string(type));
        t.links[key] = id;
    };
    const std::array<std::pair<const char*, const char*>, 4> radial{{{"W", "X"}, {"N", "X"}, {"E", "X"}, {"S", "X"}}};
    const std::array<std::pair<const char*, const char*>, 4> ring{{{"W", "N"}, {"N", "E"}, {"E", "S"}, {"S", "W"}}};
    for (const auto& [a, b] : radial) {
        draw(node.at(a), node.at(b), std::string(a) + "-" + b, std::string("Arterial ") + a + " to " + b, "arterial");
        draw(node.at(b), node.at(a), std::string(b) + "-" + a, std::string("Arterial ") + b + " to " + a, "arterial");
    }
    for (const auto& [a, b] : ring) {
        draw(node.at(a), node.at(b), std::string(a) + "-" + b, std::string("Ring road ") + a + " to " + b, "ring-road");
        draw(node.at(b), node.at(a), std::string(b) + "-" + a, std::string("Ring road ") + b + " to " + a, "ring-road");
    }
    // Each zone sits beyond its node, away from X; its access pair runs along the same bearing.
    for (const auto& z : kMacroExampleZones) {
        const auto at = node.at(z.node);
        const double len = std::hypot(at.x, at.y), ux = at.x / len, uy = at.y / len;
        const Point centroid{at.x + ux * access, at.y + uy * access};
        draw(centroid, at, std::string("in:") + z.node, std::string(z.name) + " access in", "access");
        draw(at, centroid, std::string("out:") + z.node, std::string(z.name) + " access out", "access");
        const double h = 250, cx = centroid.x + ux * h, cy = centroid.y + uy * h;
        putZone(d, {z.id, z.name, {{cx - h, cy - h}, {cx + h, cy - h}, {cx + h, cy + h}, {cx - h, cy + h}},
                    {{"households", z.households}, {"jobs", z.jobs}, {"retailJobs", z.retailJobs}}});
        putZoneConnector(d, {"", z.id, t.links.at(std::string("in:") + z.node), ZoneDirection::origin, 1});
        putZoneConnector(d, {"", z.id, t.links.at(std::string("out:") + z.node), ZoneDirection::destination, 1});
    }
    // At every node, each arriving Link to each leaving one but its reverse, both lanes across.
    for (const auto& [here, _] : node) {
        std::vector<std::pair<std::string, std::string>> arriving, leaving; // (Link key, the far end)
        for (const auto& [key, id] : t.links) {
            const auto dash = key.find('-');
            if (dash != std::string::npos) {
                const auto from = key.substr(0, dash), to = key.substr(dash + 1);
                if (to == here) arriving.push_back({key, from});
                if (from == here) leaving.push_back({key, to});
            } else if (key.substr(key.find(':') + 1) == here) {
                (key.starts_with("in:") ? arriving : leaving).push_back({key, "zone"});
            }
        }
        for (const auto& [in, from] : arriving)
            for (const auto& [out, to] : leaving) {
                if (from == to) continue;
                const auto a = t.links.at(in), b = t.links.at(out);
                const auto c = addConnectorRange(d, {a, lane(d, a, 0)}, {b, lane(d, b, 0)}, 2, 2);
                editableConnector(d, c).name = here + ": " + in + " to " + out;
            }
    }
    auto catalog = resolveDemandCatalog(*d.definition, data);
    for (auto& type : catalog.vehicleTypes)
        if (type.id == "car") type.pcu = Pcu{1.0, "A passenger car is one passenger car unit by definition"};
    putDemandCatalog(d, std::move(catalog));
    putStudyPeriod(d, {"period-am", "AM peak 07:00-08:00", 7 * 3600, 8 * 3600});
    const auto catalogs = macroExampleCatalogs();
    setTripRateSets(d, parseOwnedTripRateSets(catalogs));
    setDeterrenceSets(d, parseOwnedDeterrenceSets(catalogs));
    setCostMatrices(d, parseOwnedCostMatrices(catalogs));
    setModeChoiceSets(d, parseOwnedModeChoiceSets(catalogs));
    setPeriodFactors(d, parsePeriodFactors(catalogs));
    setProcedures(d, parseProcedures(catalogs));
    return t;
}
}
