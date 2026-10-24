// S5.2a rows SA1-SA6 (docs/reference/STUDY_AREA.md §3-§5, D327): the study area as a set of Nodes,
// Micro on its kept set with gates and typed gate flows. Each row first checks its forcing.
#include "study_area_fixture.hpp"
TEST(studyarea, sa1_byte_identity) { // SA1
    auto c = corridor();
    CHECK(c.d.zones.studyArea.nodes.empty() && documentJson(c.d)["schemaVersion"] == 54); // forcing: F4's flags make it 54
    CHECK(!documentJson(c.d).contains("studyArea"));
    const auto bytes = saved(c.d);
    CHECK(saved(parseDocument(Json::parse(bytes))) == bytes);
    auto j = Json::parse(bytes);
    j["studyArea"] = {{"nodes", {c.a}}, {"flowSource", "typed"}};
    test::throws([&] { parseDocument(j); }, "EDIT_UNSUPPORTED_FIELD");
    // No study area: compileDocument takes its old path, the whole network.
    const auto snapshot = compileDocument([&] { auto d = c.d; VehicleInput in{"", "", "car", 300, 0, 600}; in.linkId = road(d, c.aw).links[0];
        putInput(d, in); return d; }(), test::root() / "data");
    CHECK(snapshot.network.links.size() == c.d.network.links.size());
}
TEST(studyarea, sa2_codec) { // SA2
    auto c = corridor();
    auto d = c.d;
    setStudyAreaNodes(d, {c.a});
    d.zones.studyArea.gateFlows = {{"p", entryGateName("x"), exitGateName("y"), "car-only", "", 120}};
    const auto j = documentJson(d);
    CHECK(j["schemaVersion"] == 55 && j["studyArea"]["nodes"] == Json::array({c.a}) && j["studyArea"]["flowSource"] == "typed");
    CHECK(j["studyArea"]["gateFlows"][0]["compositionId"] == "car-only" && !j["studyArea"]["gateFlows"][0].contains("modeId"));
    CHECK(parseDocument(j).zones.studyArea == d.zones.studyArea);
    const auto bad = [&](auto edit) { auto x = d; edit(x); return refusal([&] { validateDocument(x); }); };
    CHECK(has(bad([](auto& x) { x.zones.studyArea.nodes.push_back("nope"); }), "STUDY_AREA_NODE_UNKNOWN", "studyArea.nodes[1]"));
    CHECK(has(bad([&](auto& x) { x.zones.studyArea.nodes.push_back(c.a); }), "STUDY_AREA_NODE_DUPLICATE", "studyArea.nodes[1]"));
    CHECK(has(bad([](auto& x) { x.zones.studyArea.gateFlows[0].periodId = "q"; }), "STUDY_AREA_PERIOD_UNKNOWN", "studyArea.gateFlows[0].periodId"));
    CHECK(has(bad([](auto& x) { x.zones.studyArea.gateFlows[0].vehiclesPerHour = -1; }), "OD_CELL_INVALID", "studyArea.gateFlows[0].vehiclesPerHour"));
    CHECK(has(bad([](auto& x) { x.zones.studyArea.gateFlows[0].compositionId.clear(); }), "OD_COMPOSITION_MISSING"));
    auto wrong = j; wrong["studyArea"]["flowSource"] = "count"; // not a source
    test::throws([&] { parseDocument(wrong); }, "EDIT_STUDY_AREA_INVALID");
    auto empty = j; empty["studyArea"]["nodes"] = Json::array();
    test::throws([&] { parseDocument(empty); }, "EDIT_STUDY_AREA_INVALID");
    setStudyAreaNodes(d, {});
    CHECK(!documentJson(d).contains("studyArea") && d.zones.studyArea.gateFlows.empty());
}
TEST(studyarea, sa3_commands) { // SA3
    auto c = corridor();
    History h; h.reset(c.d);
    CHECK(h.execute("area", [&](auto& d) { setStudyAreaNodes(d, {c.a, c.b}); }));
    const auto before = saved(h.document());
    CHECK(h.document().zones.studyArea.nodes.size() == 2); // forcing
    CHECK(h.execute("delete", [&](auto& d) { deleteNode(d, c.b); }));
    CHECK(h.document().zones.studyArea.nodes == std::vector<std::string>{c.a});
    h.undo();
    CHECK(saved(h.document()) == before);
    CHECK(h.execute("delete", [&](auto& d) { deleteNode(d, c.a); deleteNode(d, c.b); }));
    CHECK(!documentJson(h.document()).contains("studyArea"));
    h.undo();
    // A merge keeps its result in the area, even when only a merged-away Node was in it.
    auto d = c.d;
    putNode(d, {"", "C", square({150, 50}, 5), false});
    const auto extra = d.network.nodes.back().id;
    setStudyAreaNodes(d, {extra});
    History m; m.reset(d);
    CHECK(m.execute("merge", [&](auto& x) { mergeNodes(x, {c.b, extra}); }));
    CHECK(m.document().zones.studyArea.nodes == std::vector<std::string>{c.b});
}
TEST(studyarea, sa4_kept_set) { // SA4
    auto c = corridor();
    const auto& n = c.d.network;
    const auto a = studyAreaCut(n, {c.a});
    CHECK(a.issues.empty() && !a.connectors.empty()); // forcing: A has detail
    const auto links = ids(a.links);
    for (const auto& id : road(c.d, c.m3).links) CHECK(links.contains(id)); // A's middle leg
    for (const auto& id : road(c.d, c.m2).links) CHECK(!links.contains(id));
    for (const auto& id : road(c.d, c.m1).links) CHECK(!links.contains(id));
    CHECK(a.entryGates.size() == 4 && a.exitGates.size() == 4);
    const auto ab = studyAreaCut(n, {c.a, c.b});
    const auto both = ids(ab.links);
    for (const auto& r : {c.m1, c.m2, c.m3}) for (const auto& id : road(c.d, r).links) CHECK(both.contains(id));
    CHECK(ab.entryGates.size() == 6 && ab.exitGates.size() == 6);
    // B alone does not reach across the middle piece to A.
    const auto b = ids(studyAreaCut(n, {c.b}).links);
    for (const auto& id : road(c.d, c.m2).links) CHECK(!b.contains(id));
    // The cut network keeps only the kept objects, with their right-of-way.
    const auto cutA = cutNetwork(n, a, {c.a});
    CHECK(cutA.links.size() == a.links.size() && cutA.nodes.size() == 1 && cutA.roads.empty());
    CHECK(!cutA.rightOfWay.conflictAreas.empty() && cutA.rightOfWay.conflictAreas.size() < n.rightOfWay.conflictAreas.size());
}
TEST(studyarea, sa5_typed_run) { // SA5
    auto c = corridor();
    auto& d = c.d;
    VehicleInput outside{"", "", "car", 300, 0, 600}; outside.linkId = road(d, c.be).links[0]; // B's east arm, inbound
    putInput(d, outside);
    setStudyAreaNodes(d, {c.a});
    const auto cut = studyAreaCut(d.network, {c.a});
    const auto entries = ids(cut.entryGates);
    // From every entry gate to every exit gate of another arm (no U-turn is drawn): 60 veh/h each.
    const auto arm = [&](const std::string& id) { return *std::find_if(d.network.links.begin(), d.network.links.end(), [&](const auto& l) { return l.id == id; })->roadId; };
    for (const auto& in : cut.entryGates) for (const auto& out : cut.exitGates)
        if (arm(in) != arm(out)) d.zones.studyArea.gateFlows.push_back({"p", entryGateName(in), exitGateName(out), "car-only", "", 60});
    std::vector<ValidationIssue> ignored;
    const auto copy = studyAreaDocument(d, &ignored);
    CHECK(copy.network.links.size() == cut.links.size() && copy.zones.odMatrices.size() == 1); // forcing
    CHECK(ignored.size() == 1 && ignored[0].code == "MICRO_AREA_INPUT_IGNORED" && ignored[0].path == "inputs[0]");
    const auto data = test::root() / "data";
    const auto rows = runDiagnostics(d, data);
    CHECK(std::any_of(rows.begin(), rows.end(), [](const auto& r) { return r.code == "MICRO_AREA_INPUT_IGNORED"; }));
    CHECK(std::none_of(rows.begin(), rows.end(), [](const auto& r) { return r.severity == DiagnosticSeverity::runtime; }));
    const auto snapshot = compileDocument(d, data);
    CHECK(snapshot.network.links.size() == cut.links.size());
    // Every route runs from an entry gate's Link (M8.3's `od:<origin Link>><destination Link>`).
    CHECK(!snapshot.routeOrigins.empty());
    for (const auto& [runtime, authored] : snapshot.routeOrigins)
        CHECK(std::any_of(entries.begin(), entries.end(), [&](const auto& id) { return authored.rfind("od:" + id + ">", 0) == 0; }));
    const auto first = runSeed(snapshot.scenario, evaluationSpec(d, snapshot, data), 42);
    const auto again = runSeed(compileDocument(d, data).scenario, evaluationSpec(d, snapshot, data), 42);
    CHECK(first.generated > 0 && first == again);
}
TEST(studyarea, sa6_refusals) { // SA6
    auto c = corridor();
    auto coarse = c.d;
    putNode(coarse, {"", "C", square({150, 0}, 4), false}); // over the middle piece: no detail
    setStudyAreaNodes(coarse, {c.a, coarse.network.nodes.back().id});
    CHECK(has(refusal([&] { studyAreaDocument(coarse); }), "MICRO_AREA_NODE_COARSE", "studyArea.nodes[1]"));
    const auto rows = runDiagnostics(coarse, test::root() / "data");
    CHECK(std::any_of(rows.begin(), rows.end(), [](const auto& r) { return r.code == "MICRO_AREA_NODE_COARSE"; }));
    // A Node whose polygon a Link leaves and enters again.
    auto reenter = c.d;
    putNode(reenter, {"", "R", {{-2, -150}, {2, -150}, {2, 150}, {-2, 150}}, false});
    const auto cuts = nodeMembership(reenter.network, reenter.network.nodes.back()).issues;
    setStudyAreaNodes(reenter, {reenter.network.nodes.back().id});
    const auto invalid = refusal([&] { studyAreaDocument(reenter); });
    CHECK(!cuts.empty() && has(invalid, "MICRO_AREA_NODE_INVALID", "studyArea.nodes[0]") && has(invalid, cuts.front()));
    auto gate = c.d;
    setStudyAreaNodes(gate, {c.a});
    gate.zones.studyArea.gateFlows = {{"p", entryGateName(road(gate, c.m1).links[0]), exitGateName("nope"), "car-only", "", 60}};
    const auto unknown = refusal([&] { studyAreaDocument(gate); });
    CHECK(has(unknown, "STUDY_AREA_GATE_UNKNOWN", "studyArea.gateFlows[0].from") && has(unknown, "STUDY_AREA_GATE_UNKNOWN", "studyArea.gateFlows[0].to"));
}
TEST(studyarea, sa16_gate_flow_commands) { // SA16 (S5.2b)
    auto c = corridor();
    auto& d = c.d;
    setStudyAreaNodes(d, {c.a});
    const auto cut = studyAreaCut(d.network, {c.a});
    // A zone whose origin connector stands on a kept Link joins the gates, after them.
    const auto in = cut.entryGates.front();
    d.zones.zones.push_back({"zone-in", "Inside", square({0, 50}, 2), {}});
    d.zones.zoneConnectors.push_back({"zc-1", "zone-in", in, ZoneDirection::origin, 1});
    const auto ends = studyAreaEnds(d);
    CHECK(ends.origins.size() == cut.entryGates.size() + 1 && ends.origins.front() == entryGateName(cut.entryGates.front()));
    CHECK(ends.origins.back() == "zone-in" && ends.destinations.size() == cut.exitGates.size());
    // setGateFlows replaces one key only, and drops zeros.
    d.zones.studyPeriods.push_back({"q", "Q", 600, 1200});
    setGateFlows(d, "p", "car-only", "", {{"", ends.origins[0], ends.destinations[0], "", "", 60}, {"", ends.origins[1], ends.destinations[0], "", "", 0}});
    setGateFlows(d, "q", "car-only", "", {{"", ends.origins[0], ends.destinations[0], "", "", 30}});
    CHECK(d.zones.studyArea.gateFlows.size() == 2); // forcing: two keys
    setGateFlows(d, "p", "car-only", "", {{"", ends.origins[1], ends.destinations[1], "", "", 45}});
    CHECK(d.zones.studyArea.gateFlows.size() == 2 && d.zones.studyArea.gateFlows[0].periodId == "q" &&
          d.zones.studyArea.gateFlows[1].vehiclesPerHour == 45 && d.zones.studyArea.gateFlows[1].periodId == "p");
    validateDocument(d);
    CHECK(studyAreaDocument(d).zones.odMatrices.size() == 2);
    setStudyAreaGateFlows(d, {{"p", ends.origins[0], ends.destinations[0], "car-only", "", 0}});
    CHECK(d.zones.studyArea.gateFlows.empty());
}
TEST(studyarea, sa7_path_decomposition) { // SA7 (S5.3)
    const MacroClass cls{"c", "", 1, {{0, 1, 2000}}};
    const auto r = assignMacro(pair(1), {cls}, {1e-4, 1000}, {}, true);
    CHECK(r.converged && r.iterations == 1 && r.paths.size() == 1 && r.paths[0].size() == 2); // forcing: one step, two paths
    // By hand: free flow loads 2000 on link 0; the one step lambda lands on x1 = 5.6 / 0.0033, so the
    // start's path keeps 2000 (1 - lambda) = x1 and the step's path carries 2000 lambda = 2000 - x1.
    const double x1 = 5.6 / 0.0033;
    CHECK(r.paths[0][0].links == std::vector<std::size_t>{0} && r.paths[0][1].links == std::vector<std::size_t>{1});
    CHECK(std::abs(r.paths[0][0].vehiclesPerHour - x1) / x1 < 1e-3 && std::abs(r.paths[0][1].vehiclesPerHour - (2000 - x1)) / x1 < 1e-3);
    // Three routes at BPR 4 take many steps; the paths still sum to the link flows.
    const auto three = pair(4, {10, 12, 14});
    const auto r4 = assignMacro(three, {{"c", "", 1, {{0, 1, 3000}}}}, {1e-6, 1000}, {}, true);
    CHECK(r4.iterations > 1 && r4.paths[0].size() == 3);
    for (std::size_t a = 0; a < 3; ++a) {
        double sum = 0;
        for (const auto& p : r4.paths[0]) if (std::find(p.links.begin(), p.links.end(), a) != p.links.end()) sum += p.vehiclesPerHour;
        test::near(sum, r4.classFlows[0][a], 1e-9);
    }
    // Without keepPaths nothing else changes.
    auto plain = assignMacro(three, {{"c", "", 1, {{0, 1, 3000}}}}, {1e-6, 1000});
    CHECK(plain.paths.empty());
    plain.paths = r4.paths;
    CHECK(plain == r4);
}
TEST(studyarea, sa8_macro_cut) { // SA8 (S5.3)
    auto c = demandCorridor(100); // 600 veh/h over 600 s
    auto& d = c.d;
    setStudyAreaNodes(d, {c.a});
    d.zones.studyArea.flowSource = "macro";
    const auto data = test::root() / "data";
    std::vector<ValidationIssue> advisories;
    const auto flows = macroGateFlows(d, data, &advisories);
    CHECK(flows.size() == 1); // forcing: one trip pair
    const auto outbound = road(d, c.m3).links[1]; // A's middle leg, eastbound
    CHECK(flows[0].from == "west" && flows[0].to == exitGateName(outbound) && flows[0].periodId == "p" && flows[0].compositionId == "car-only");
    test::near(flows[0].vehiclesPerHour, 600, 1e-6);
    setStudyAreaNodes(d, {c.a, c.b});
    const auto both = macroGateFlows(d, data);
    CHECK(both.size() == 1 && both[0].from == "west" && both[0].to == "east");
    test::near(total(both), 600, 1e-6);
    // Leaving the kept set and entering it again makes two trips (a synthetic path).
    MacroGraph g; g.nodes = {"0", "1", "2", "3"};
    for (const auto& id : {"L1", "X", "L2"}) { MacroLink l; l.source = id; g.links.push_back(l); }
    const StudyAreaEnds ends{{entryGateName("L1"), entryGateName("L2")}, {exitGateName("L1"), exitGateName("L2")}};
    std::size_t unmatched = 0;
    const auto trips = cutTrips(g, {0, 1, 2}, {"L1", "L2"}, {}, d, ends, &unmatched);
    CHECK(unmatched == 0 && trips == (std::vector<CutTrip>{{entryGateName("L1"), exitGateName("L1")}, {entryGateName("L2"), exitGateName("L2")}}));
    CHECK(cutTrips(g, {0, 1, 2}, {"L1", "L2"}, {}, d, {{entryGateName("L1")}, {exitGateName("L1")}}, &unmatched).size() == 1 && unmatched == 1);
}
TEST(studyarea, sa9_macro_periods) { // SA9 (S5.3)
    auto c = demandCorridor(100);
    auto& d = c.d;
    d.zones.studyPeriods.push_back({"q", "Q", 600, 1200});
    putOdMatrix(d, {"pm", 0, 0, "car-only", {"west", "east"}, {{0, 50}, {0, 0}}, "q"});
    setStudyAreaNodes(d, {c.a});
    d.zones.studyArea.flowSource = "macro";
    const auto data = test::root() / "data";
    const auto flows = macroGateFlows(d, data);
    CHECK(flows.size() == 2 && flows[0].periodId == "p" && flows[1].periodId == "q"); // forcing: two periods
    test::near(flows[0].vehiclesPerHour, 600, 1e-6); test::near(flows[1].vehiclesPerHour, 300, 1e-6);
    // A period partly past the run (1500 s) is left out and said so; none inside refuses.
    auto late = d; late.zones.studyPeriods.push_back({"r", "R", 1200, 1800});
    std::vector<ValidationIssue> advisories;
    macroGateFlows(late, data, &advisories);
    CHECK(has(advisories, "MICRO_AREA_PERIOD_OUTSIDE", "studyPeriods[2]"));
    auto none = d; none.zones.odMatrices.clear(); none.zones.studyPeriods = {{"z", "Z", 2000, 2600}};
    CHECK(has(refusal([&] { macroGateFlows(none, data); }), "MICRO_AREA_PERIOD_NOT_RUN", "studyPeriods"));
    // (MACRO_NOT_CONVERGED's pass-through needs two routes; this corridor has one, so its gap is 0.)
    // The area run from Macro starts and repeats its bytes.
    const auto rows = runDiagnostics(d, data);
    CHECK(std::none_of(rows.begin(), rows.end(), [](const auto& r) { return r.severity == DiagnosticSeverity::runtime; }));
    const auto snapshot = compileDocument(d, data);
    const auto first = runSeed(snapshot.scenario, evaluationSpec(d, snapshot, data), 42);
    const auto again = runSeed(compileDocument(d, data).scenario, evaluationSpec(d, snapshot, data), 42);
    CHECK(first.generated > 0 && first == again);
}
TEST(studyarea, sa10_meso_watched_counts) { // SA10 (S5.4)
    auto c = mesoCorridor(100);
    auto& d = c.d;
    const auto cut = studyAreaCut(d.network, {c.a});
    const auto data = test::root() / "data";
    const auto run = runMeso(d, data, nullptr, nullptr, &cut.links);
    CHECK(!run.loading.watched.empty()); // forcing: the corridor's path crosses A
    // On each watched Meso link, the paths' entrance counts sum to its aggregate entrance count.
    std::map<std::size_t, std::vector<const MesoLoading::Watched*>> byLink;
    for (const auto& w : run.loading.watched) byLink[run.classes[w.cls].paths[w.path].links[w.position]].push_back(&w);
    for (const auto& [l, ws] : byLink) {
        const auto& up = run.loading.up[l].at;
        for (std::size_t k = 0; k < up.size(); ++k) {
            double sum = 0;
            for (const auto* w : ws) { sum += w->entrance.at[k]; if (k) CHECK(w->entrance.at[k] >= w->entrance.at[k - 1] - 1e-12); }
            test::near(sum, up[k], 1e-9);
        }
    }
    // Without a watch the loading is the same and keeps no counts.
    auto plain = runMeso(d, data);
    CHECK(plain.loading.watched.empty() && plain.loading.up.size() == run.loading.up.size());
    for (std::size_t l = 0; l < run.loading.up.size(); ++l)
        CHECK(plain.loading.up[l].at == run.loading.up[l].at && plain.loading.down[l].at == run.loading.down[l].at);
}
TEST(studyarea, sa11_meso_cut) { // SA11 (S5.4)
    auto c = mesoCorridor(100);
    auto& d = c.d;
    setStudyAreaNodes(d, {c.a});
    d.zones.studyArea.flowSource = "meso";
    const auto data = test::root() / "data";
    std::vector<ValidationIssue> advisories;
    const auto flows = mesoGateFlows(d, data, &advisories);
    CHECK(!flows.empty()); // forcing
    const auto outbound = road(d, c.m3).links[1];
    double vehicles = 0;
    for (const auto& f : flows) {
        CHECK(f.periodId.empty() && f.from == "west" && f.to == exitGateName(outbound) && f.compositionId == "car-only");
        CHECK(std::fmod(f.start, 300) == 0 && f.end > f.start && f.end <= 1500); // on the 300 s slice grid of the 1500 s run
        vehicles += f.vehiclesPerHour * (f.end - f.start) / 3600;
    }
    // Every vehicle the path took onto its first kept Link is a trip: here all 100, inside the run.
    const auto keptA = studyAreaCut(d.network, {c.a}).links;
    const auto run = runMeso(d, data, nullptr, nullptr, &keptA);
    double entered = 0;
    for (const auto& w : run.loading.watched) if (w.position == 0) entered += w.entrance.at.back() / run.classes[w.cls].pcu;
    test::near(vehicles, entered, 1e-9);
    CHECK(entered > 99 && entered < 100 + 1e-9);
    // B too: the trip runs from zone to zone.
    setStudyAreaNodes(d, {c.a, c.b});
    const auto both = mesoGateFlows(d, data);
    CHECK(!both.empty() && std::all_of(both.begin(), both.end(), [](const auto& f) { return f.from == "west" && f.to == "east"; }));
    // The area run from Meso starts and repeats its bytes.
    setStudyAreaNodes(d, {c.a});
    const auto rows = runDiagnostics(d, data);
    CHECK(std::none_of(rows.begin(), rows.end(), [](const auto& r) { return r.severity == DiagnosticSeverity::runtime; }));
    const auto snapshot = compileDocument(d, data);
    CHECK(studyAreaDocument(d, nullptr, data).zones.odMatrices.size() == flows.size()); // one matrix per slice
    const auto first = runSeed(snapshot.scenario, evaluationSpec(d, snapshot, data), 42);
    const auto again = runSeed(compileDocument(d, data).scenario, evaluationSpec(d, snapshot, data), 42);
    CHECK(first.generated > 0 && first == again);
}
TEST(studyarea, sa12_side_files) { // SA12 (S5.5a)
    auto c = mesoCorridor(100);
    auto& d = c.d;
    d.zones.studyPeriods.push_back({"q", "Q", 600, 1200});
    putOdMatrix(d, {"pm", 0, 0, "car-only", {"west", "east"}, {{0, 50}, {0, 0}}, "q"});
    setStudyAreaNodes(d, {c.a});
    const auto data = test::root() / "data";
    const auto sha = projectSha256(d), bytes = documentBytes(d);
    CHECK(sha.size() == 64 && sha == inputSha256(bytes)); // forcing: the hash is of the bytes saving writes
    const auto p = runMacro(d, data, "p", nullptr, nullptr, true);
    const auto first = macroSideJson(p, sha, Json());
    auto side = parseMacroSide(first);
    CHECK(side.periods.size() == 1 && side.periods.at("p").hasPaths && side.periods.at("p").paths == p.assignment.paths);
    CHECK(side.periods.at("p").arcs == sideArcs(p.graph) && side.periods.at("p").projectSha256 == sha);
    CHECK(Json::parse(first.dump()) == first); // the bytes read back equal
    // A second period joins the file; its first period is kept as it was.
    const auto both = macroSideJson(runMacro(d, data, "q"), sha, first);
    side = parseMacroSide(both);
    CHECK(side.periods.size() == 2 && both["periods"]["p"] == first["periods"]["p"] && !side.periods.at("q").hasPaths);
    // Meso: the watched counts, per (class, path, position), read back equal.
    const auto kept = studyAreaCut(d.network, {c.a}).links;
    const auto m = runMeso(d, data, nullptr, nullptr, &kept);
    const auto meso = parseMesoSide(mesoSideJson(m, sha, kept));
    CHECK(!m.loading.watched.empty() && meso.watched.size() == m.loading.watched.size() && meso.watchedLinks == kept);
    for (std::size_t i = 0; i < meso.watched.size(); ++i)
        CHECK(meso.watched[i].cls == m.loading.watched[i].cls && meso.watched[i].path == m.loading.watched[i].path &&
              meso.watched[i].entrance.at == m.loading.watched[i].entrance.at);
    CHECK(meso.links == sideArcs(m.graph) && meso.duration == m.duration && meso.settings == m.settings);
    CHECK(documentBytes(d) == bytes); // nothing here touched the project
    CHECK(sideFilePath("/x/net.traffic.json", "macro") == std::filesystem::path("/x/net.traffic.macro.json"));
}
TEST(studyarea, sa13_side_file_cut) { // SA13 (S5.5a)
    auto c = mesoCorridor(100);
    auto& d = c.d;
    setStudyAreaNodes(d, {c.a});
    d.zones.studyArea.flowSource = "macro";
    const auto data = test::root() / "data";
    const auto atCompile = macroGateFlows(d, data);
    const auto file = std::make_shared<const Json>(macroSideJson(runMacro(d, data, "p", nullptr, nullptr, true), projectSha256(d), Json()));
    const StudyAreaSources sources{file, nullptr, inputSha256(file->dump()), {}};
    const auto cut = [&](const ProjectDocument& doc, const StudyAreaSources& s, StudyAreaProvenance& from, std::vector<ValidationIssue>& advisories) {
        from = {"macro", {}, projectSha256(doc), {}, false};
        return macroGateFlows(doc, data, &advisories, &s, &from);
    };
    StudyAreaProvenance from; std::vector<ValidationIssue> advisories;
    const auto fresh = cut(d, sources, from, advisories);
    CHECK(from.periods == std::vector<std::string>{"p"} && from.sourceSha256 == sources.macroSha256 && !from.stale); // forcing: read from the file
    CHECK(fresh.size() == atCompile.size() && !has(advisories, "MICRO_AREA_SOURCE_STALE"));
    for (std::size_t i = 0; i < fresh.size(); ++i) CHECK(fresh[i].from == atCompile[i].from && std::abs(fresh[i].vehiclesPerHour - atCompile[i].vehiclesPerHour) < 1e-9);
    // Other project bytes: still the file's flows, marked stale.
    auto renamed = d; renamed.network.links[0].name = "Renamed";
    advisories.clear();
    cut(renamed, sources, from, advisories);
    CHECK(from.stale && from.periods.size() == 1 && has(advisories, "MICRO_AREA_SOURCE_STALE", "studyArea.flowSource"));
    // Another graph, or no paths: run at compile, stale (owner); the flows are the run's.
    auto other = *file; other["periods"]["p"]["arcs"][0][1] = "elsewhere";
    for (const auto& j : {other, macroSideJson(runMacro(d, data, "p"), projectSha256(d), Json())}) {
        advisories.clear();
        const auto ran = cut(d, {std::make_shared<const Json>(j), nullptr, "x", {}}, from, advisories);
        CHECK(from.stale && from.periods.empty() && from.sourceSha256.empty() && has(advisories, "MICRO_AREA_SOURCE_STALE"));
        CHECK(ran.size() == atCompile.size() && std::abs(total(ran) - total(atCompile)) < 1e-9);
    }
    // Not a side file: refused at its path.
    auto bad = *file; bad["periods"]["p"]["paths"][0][0]["links"][0] = 9999;
    CHECK(has(refusal([&] { const StudyAreaSources s{std::make_shared<const Json>(bad), nullptr, "", ""}; macroGateFlows(d, data, nullptr, &s); }),
              "MICRO_AREA_SOURCE_INVALID", "macroSide.periods.p.paths[0][0].links[0]"));
    CHECK(has(refusal([&] { const StudyAreaSources s{std::make_shared<const Json>(nullptr), nullptr, "", ""}; macroGateFlows(d, data, nullptr, &s); }),
              "MICRO_AREA_SOURCE_INVALID", "macroSide"));
    // The run records where its flows came from; a changed file changes the record.
    const auto snapshot = compileDocument(d, data, nullptr, &sources);
    CHECK(snapshot.studyArea && snapshot.studyArea->sourceSha256 == sources.macroSha256 && !compileDocument(d, data).studyArea->stale);
    // Meso: a file that watched the kept Links is cut; one that watched nothing runs at compile, stale.
    d.zones.studyArea.flowSource = "meso";
    const auto kept = studyAreaCut(d.network, {c.a}).links;
    const auto m = runMeso(d, data, nullptr, nullptr, &kept);
    const auto mesoAtCompile = mesoGateFlows(d, data);
    for (const auto& watched : {kept, std::vector<std::string>{}}) {
        const StudyAreaSources s{nullptr, std::make_shared<const Json>(mesoSideJson(m, projectSha256(d), watched)), {}, "m"};
        from = {"meso", {}, projectSha256(d), {}, false}; advisories.clear();
        const auto flows = mesoGateFlows(d, data, &advisories, &s, &from);
        CHECK(from.stale == watched.empty() && from.sourceSha256 == (watched.empty() ? "" : "m") && has(advisories, "MICRO_AREA_SOURCE_STALE") == watched.empty());
        CHECK(flows.size() == mesoAtCompile.size() && std::abs(total(flows) - total(mesoAtCompile)) < 1e-9);
    }
}
