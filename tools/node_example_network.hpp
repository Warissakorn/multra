#pragma once
// The Node analysis example (data/projects/signalised-node-analysis.traffic.json): one signalised
// four-leg junction that shows both letters the program can give out of the box. *Analyse* in
// Results -> Node (HCM) gives the HCM 7 Chapter 19 table from the turning counts stored on the Node;
// *Run seeds* gives LOS from simulated section delay, one travel-time section per movement. The two
// are different quantities ([LOS](../docs/reference/LOS.md), [NODE_ANALYSIS](../docs/reference/NODE_ANALYSIS.md)).
// Built the way the editor builds a junction today: two Street segments, Find crossings, a junction
// template and Fill. `nodeexample` fails until the committed file is this builder's output.
// Regenerate with multra-node-example, never by hand.
//
// Layout, left-hand traffic, metres: an east-west major and a north-south minor road, two lanes each
// way, crossing at (0, 0) with 300 m arms. Template `signal-4-p3-shared`: on every approach one lane
// for the far turn and one shared by through and the kerb turn, every turn protected (in scope for
// Chapter 19's core method). No pocket: when this was built a placed routing decision lost the far
// turn a pocket serves (fixed since, D501); moving to the pocket template is the owner's choice.
//
// Every number here (volumes, timings, dimensions) is an illustration chosen so that every lane
// group runs below capacity; none is a count, and no output is a claim about a real junction (rule 4).
#include "../src/commands/demand_commands.hpp"
#include "../src/commands/fill_commands.hpp"
#include "../src/commands/junction_commands.hpp"
#include "../src/commands/node_analysis_commands.hpp"
#include "../src/commands/node_commands.hpp"
#include "../src/commands/right_of_way_commands.hpp"
#include "../src/commands/street_segment_commands.hpp"
#include "../src/model/network/coarse_nodes.hpp"
#include "../src/model/network/node_membership.hpp"
#include "../src/project/junction_template_codec.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

namespace multra::fixture {
inline constexpr const char* kNodeExampleCitation =
    "Illustrative values for the shipped Node analysis example only (data/projects/README.md); not a count, not a study";
inline constexpr double kNodeExampleWarmup = 300, kNodeExampleEnd = 1200, kNodeExampleCooldown = 300;

// One approach: where its traffic comes from, and its hourly volume by turn.
struct NodeExampleApproach {
    const char* name;      // the side of the junction it arrives from
    double through, far, kerb; // veh/h; far = across the opposing stream (right, in left-hand traffic)
};
// Compass order W, E (major), S, N (minor), matched to Links by where each entry Link starts.
inline constexpr std::array<NodeExampleApproach, 4> kNodeExampleApproaches{{
    {"West", 450, 120, 80}, {"East", 420, 110, 70}, {"South", 260, 70, 60}, {"North", 240, 60, 70}}};

struct NodeExample {
    ProjectDocument document;
    std::array<std::string, 4> entries, exits; // Link ids in kNodeExampleApproaches' order (an exit leaves toward that side)
};

inline NodeExample nodeExample(const std::filesystem::path& data) {
    constexpr double arm = 300;
    ProjectDocument d;
    d.network.id = "signalised-node-analysis";
    d.network.drivingSide = DrivingSide::left;
    d.definition = AuthoringDefinition{};
    auto& def = *d.definition;
    putCrossSection(d, {"urban-2x2", "Urban street, two lanes each way (example)", 2, 2, 2, 3.5, {}, 8, std::nullopt,
                        kNodeExampleCitation, 50}, {});
    putStreetSegment(d, {"major", "Major road (east-west)", {{-arm, 0}, {arm, 0}}, std::string("urban-2x2"), 0, {}, {}});
    putStreetSegment(d, {"minor", "Minor road (north-south)", {{0, -arm}, {0, arm}}, std::string("urban-2x2"), 0, {}, {}});
    const auto places = findCrossings(d.network, 2);
    if (places.size() != 1) throw std::logic_error("node example: one crossing expected");
    const auto node = applyCrossings(d, places, {CrossingChoice::junction}, 2, def.crossSections).at(0);
    // Room for the turns, as an author would draw the junction area.
    putNode(d, {node, "Example junction", {{-30, -30}, {30, -30}, {30, 30}, {-30, 30}}, false});
    for (const auto& s : d.network.streetSegments) applyTemplate(d, s.id, def.crossSections);

    NodeFillCatalogs catalogs{def.crossSections, {}, {3.0, 7.0}};
    for (const auto& t : resolveJunctionTemplates(def, data)) catalogs.junctionTemplates.push_back(t.type);
    // NEMA phases: 2/6 major through, 1/5 major far turns, 4/8 minor through, 3/7 minor far turns.
    NodeJunction recipe;
    recipe.templateId = "signal-4-p3-shared";
    recipe.signal = JunctionSignal{};
    for (const auto& [number, green] : std::array<std::pair<int, double>, 8>{{{1, 11}, {2, 40}, {3, 9}, {4, 24}, {5, 11}, {6, 40}, {7, 9}, {8, 24}}})
        recipe.signal->phases.push_back({number, green, 3, 1});
    recipe.signal->cycle = 100;
    for (const auto& s : d.network.streetSegments)
        if (s.centreline.front().y == 0 && s.centreline.back().y == 0) recipe.major.push_back(s.id);
    setNodeJunction(d, node, recipe);

    NodeExample t;
    // Entry and exit Links by the side of the junction they touch.
    const auto side = [](const Point& p) { return std::abs(p.x) > std::abs(p.y) ? (p.x < 0 ? 0 : 1) : (p.y < 0 ? 2 : 3); };
    for (const auto& leg : nodeLinkLegs(d.network, d.network.nodes.front()))
        for (const auto& l : d.network.links)
            if (l.id == leg.linkId) (leg.in ? t.entries : t.exits)[side(leg.in ? l.geometry.front() : l.geometry.back())] = l.id;

    History history;
    history.reset(d);
    auto offers = fillOffers(history.document(), catalogs);
    std::erase_if(offers, [](const auto& o) { return o.kind != FillKind::junction; });
    if (offers.size() != 1 || !offers[0].refusal.empty()) throw std::logic_error("node example: the junction does not fill");
    history.execute("editorFillDefaults", [&](auto& x) { applyFills(x, offers, catalogs); }, true);
    d = history.document();

    changeRunSettings(d, kNodeExampleEnd, 0.1);
    changeEvaluationPeriod(d, kNodeExampleWarmup, kNodeExampleEnd, kNodeExampleCooldown);
    // From W the far (right) turn leaves to the south and the kerb (left) turn to the north, and so on round.
    constexpr std::array<std::size_t, 4> farSide{2, 3, 1, 0}, kerbSide{3, 2, 0, 1};
    NodeAnalysis analysis;
    analysis.analysisPeriod = (kNodeExampleEnd - kNodeExampleWarmup) / 3600; // 0.25 h, the evaluation window
    for (std::size_t a = 0; a < 4; ++a) {
        const auto& approach = kNodeExampleApproaches[a];
        const std::size_t opposite = a ^ 1U;
        const std::array<std::pair<std::size_t, double>, 3> turns{{{opposite, approach.through}, {farSide[a], approach.far}, {kerbSide[a], approach.kerb}}};
        VehicleInput input{"", "", "car", approach.through + approach.far + approach.kerb, 0, kNodeExampleEnd};
        input.linkId = t.entries[a];
        putInput(d, input);
        RoutingDecision decision{"", std::string(approach.name) + " approach turns", {}, t.entries[a], {}, {}, 20.0};
        const std::array<const char*, 3> movement{"through", "right", "left"};
        for (std::size_t k = 0; k < 3; ++k) {
            const auto& [to, volume] = turns[k];
            decision.routes.push_back({"", volume, t.exits[to], {}});
            putTravelTimeSection(d, {"", std::string(approach.name) + " " + movement[k], {t.entries[a], 60}, {t.exits[to], 40},
                                     SectionControl::signalised});
        }
        putRoutingDecision(d, decision);
    }
    // The same volumes as turning counts on the Node, named by its derived legs.
    const auto membership = nodeMembership(d.network, d.network.nodes.front());
    if (!membership.issues.empty() || membership.turns.size() != 12) throw std::logic_error("node example: twelve turns expected");
    for (const auto& turn : membership.turns) {
        const auto& in = membership.inLegs[turn.in].linkId; const auto& out = membership.outLegs[turn.out].linkId;
        const auto link = std::find_if(d.network.links.begin(), d.network.links.end(), [&](const auto& l) { return l.id == in; });
        const std::size_t a = side(link->geometry.back()), to = std::find(t.exits.begin(), t.exits.end(), out) - t.exits.begin();
        const auto& approach = kNodeExampleApproaches[a];
        analysis.turns.push_back({in, out, to == (a ^ 1U) ? approach.through : to == farSide[a] ? approach.far : approach.kerb, 0.0});
    }
    setNodeAnalysis(d, node, analysis);
    t.document = std::move(d);
    return t;
}
}
