// UTDF U1 (UTDF_EXPORT §7 U1 contract, §10 UT0-UT5 and UT11's geometry): the writer's geometry
// sections, the Node `number` (schema 65), and the section names against Vistro's golden file.
#include "test.hpp"
#include "../src/commands/connector_commands.hpp"
#include "../src/commands/demand_commands.hpp"
#include "../src/commands/network_commands.hpp"
#include "../src/commands/pocket_commands.hpp"
#include "../src/model/demand/nema.hpp"
#include "../src/project/document_schema.hpp"
#include "../src/project/utdf_export.hpp"
#include <fstream>
#include <map>
#include <set>
#include <sstream>
using namespace multra;
namespace {
std::vector<std::string> lines(const std::string& csv) {
    std::vector<std::string> out; std::string line; std::istringstream s(csv);
    while (std::getline(s, line)) { if (!line.empty() && line.back() == '\r') line.pop_back(); out.push_back(line); }
    return out;
}
std::string file(const std::string& name) {
    std::ifstream f(test::root() / "tests/data/utdf" / name, std::ios::binary);
    std::ostringstream s; s << f.rdbuf(); return s.str();
}
// Section -> (header columns, record names): the first cell of every row after the header.
std::map<std::string, std::pair<std::vector<std::string>, std::set<std::string>>> sections(const std::string& csv) {
    std::map<std::string, std::pair<std::vector<std::string>, std::set<std::string>>> out;
    std::string section; int row = 0;
    for (const auto& line : lines(csv)) {
        if (line.empty()) continue;
        if (line.front() == '[') { section = line; row = 0; continue; }
        std::vector<std::string> cells; std::string cell; std::istringstream s(line);
        while (std::getline(s, cell, ',')) cells.push_back(cell);
        if (++row == 1) continue; // the title line
        if (row == 2) out[section].first = cells; else if (!cells.empty()) out[section].second.insert(cells.front());
    }
    return out;
}
bool has(const std::vector<UtdfNote>& report, const std::string& code, const std::string& text = {}) {
    return std::any_of(report.begin(), report.end(), [&](const auto& n) { return n.code == code && n.text.find(text) != std::string::npos; });
}
bool contains(const std::string& csv, const std::string& line) {
    const auto all = lines(csv); return std::find(all.begin(), all.end(), line) != all.end();
}
// UTDF_PROBE §1's junction: Main St E-W 60 km/h with far pockets 60/20 m, Side St N-S 50 km/h with kerb
// pockets 40/15 m, 3.25 m lanes, legs 300 m from the centre at (1000, 1000), left-hand traffic.
// Without Street segments (the CLI fixture, which must validate) the names sit on the Links and no
// median is known.
ProjectDocument u0Junction(bool segments = true) {
    ProjectDocument d; d.network.drivingSide = DrivingSide::left;
    if (segments) d.network.streetSegments = {{"main", "Main St", {}, std::nullopt, 0, {std::nullopt, std::nullopt, std::nullopt, 2.0}, {}},
                                {"side", "Side St", {}, std::nullopt, 0, {std::nullopt, std::nullopt, std::nullopt, 0.0}, {}}};
    const std::array<Point, 4> outward{{{0, 1}, {1, 0}, {0, -1}, {-1, 0}}}; // legs 1 N, 2 E, 3 S, 4 W
    for (std::size_t k = 0; k < 4; ++k) {
        const auto [x, y] = outward[k]; const auto leg = "leg" + std::to_string(k + 1);
        const bool main = k % 2 == 1;
        for (const bool in : {true, false}) {
            Link l{leg + (in ? "-in" : "-out"),
                   in ? std::vector<Point>{{1000 + 300 * x, 1000 + 300 * y}, {1000 + 10 * x, 1000 + 10 * y}}
                      : std::vector<Point>{{1000 + 10 * x, 1000 + 10 * y}, {1000 + 300 * x, 1000 + 300 * y}},
                   {{leg + (in ? "-i" : "-o") + "k", 3.25}, {leg + (in ? "-i" : "-o") + "f", 3.25}}};
            if (segments) l.streetSegmentId = main ? "main" : "side"; else l.name = main ? "Main St" : "Side St";
            l.speedLimit = main ? 60. : 50.;
            d.network.links.push_back(l);
        }
    }
    const auto turn = [](std::size_t from, int quarter) { return (from + 2 + 4 - quarter) % 4; }; // outward leg index
    // UTDF_PROBE §3: 100 s, 2/6 0-40, 4/8 44-82, leading far turns 1/5 86-96, 3 s yellow, 1 s all-red.
    d.definition.emplace();
    const auto controller = putSignalController(d, {"u0-signal", "U0", 100, 0,
        {{1, "EBR", 86, 96, 3}, {2, "WB", 0, 40, 3}, {4, "SB", 44, 82, 3}, {5, "WBR", 86, 96, 3}, {6, "EB", 0, 40, 3}, {8, "NB", 44, 82, 3}}});
    const std::array<int, 4> through{4, 2, 8, 6}, far{4, 5, 8, 1}; // by origin leg: SB, WB, NB, EB
    const std::array<std::array<double, 3>, 4> volumes{{{80, 280, 120}, {100, 850, 180}, {90, 300, 110}, {120, 900, 200}}}; // L, T, R
    std::vector<NodeAnalysisTurn> turns;
    std::vector<std::string> stopLinks;
    for (std::size_t k = 0; k < 4; ++k) {
        const bool main = k % 2 == 1; const auto in = "leg" + std::to_string(k + 1) + "-in";
        const auto stop = addTurnPocket(d, in, main ? 60 : 40, main ? 20 : 15, main ? PocketSide::far : PocketSide::kerb);
        auto& l = editableLink(d, stop); l.speedLimit = main ? 60. : 50.;
        if (segments) l.streetSegmentId = main ? "main" : "side"; else l.name = main ? "Main St" : "Side St";
        stopLinks.push_back(stop);
        const auto lanes = l.lanes; // kerb to far
        const auto out = [&](int quarter) { return "leg" + std::to_string(turn(k, quarter) + 1) + "-out"; };
        const auto lane = [&](int quarter, std::size_t i) { return editableLink(d, out(quarter)).lanes[i].id; };
        // Left (+1 quarter) is the kerb turn in left-hand traffic, right (-1) the far turn.
        const std::size_t ahead = main ? 0 : 1;
        const auto t = addConnectorRange(d, {stop, lanes[ahead].id}, {out(0), lane(0, 0)}, 2, 2);
        const auto kerb = addConnectorRange(d, {stop, lanes[0].id}, {out(1), lane(1, 0)}, 1, 1);
        const auto right = addConnectorRange(d, {stop, lanes[2].id}, {out(-1), lane(-1, 1)}, 1, 1);
        for (const auto& [connector, group] : std::vector<std::pair<std::string, int>>{{t, through[k]}, {kerb, through[k]}, {right, far[k]}})
            putSignalHead(d, {"", {}, 0.5, "", connector, "", controller, group});
        for (const auto [quarter, i] : std::vector<std::pair<int, std::size_t>>{{1, 0}, {0, 1}, {-1, 2}})
            turns.push_back({stop, out(quarter), volumes[k][i], 5.});
    }
    Node node{"junction", "Main St & Side St", {{970, 970}, {1030, 970}, {1030, 1030}, {970, 1030}}, false};
    node.number = 1; node.analysis = NodeAnalysis{};
    node.analysis->peakHourFactor = 0.92; node.analysis->baseSaturationFlow = SaturationFlowSource{1900, "UTDF_PROBE sheet"};
    node.analysis->turns = turns;
    for (const auto& s : stopLinks) node.analysis->approaches.push_back({s, 0});
    d.network.nodes.push_back(node);
    return d;
}
// Two Nodes 300 m apart on one eastbound Link chain, numbered 10 and 20, no names or analysis.
ProjectDocument corridor() {
    ProjectDocument d;
    d.network.links = {{"up", {{-200, 0}, {-10, 0}}, {{"u0", 3.5}}}, {"between", {{10, 0}, {290, 0}}, {{"b0", 3.5}}},
                       {"exit", {{310, 0}, {400, 0}}, {{"e0", 3.5}}}};
    for (auto& l : d.network.links) l.speedLimit = 50.;
    addConnectorRange(d, {"up", "u0"}, {"between", "b0"}, 1, 1);
    addConnectorRange(d, {"between", "b0"}, {"exit", "e0"}, 1, 1);
    d.network.nodes = {{"a", "A", {{-30, -30}, {30, -30}, {30, 30}, {-30, 30}}, false},
                       {"b", "B", {{270, -30}, {330, -30}, {330, 30}, {270, 30}}, false}};
    d.network.nodes[0].number = 10; d.network.nodes[1].number = 20;
    return d;
}
}
TEST(utdf, ut1_ut6_ut7_u0_junction_matches_the_handwritten_file_vistro_imports) {
    const auto d = u0Junction();
    const auto layout = approachLayout(d.network, d.network.nodes[0]);
    CHECK(layout.issues.empty() && layout.movements.size() == 12); // forcing: a valid Thai four-leg layout
    CHECK(nemaCheck(d.definition->signalControllers[0], {d.network, layout}).empty()); // forcing: UTDF_PROBE §3 is NEMA
    const auto e = utdfDocument(d);
    CHECK(e.csv.find("\r\n") != std::string::npos && e.csv.rfind("\xEF\xBB\xBF", 0) == std::string::npos);
    CHECK(std::count(e.csv.begin(), e.csv.end(), '\n') == std::count(e.csv.begin(), e.csv.end(), '\r'));
    CHECK(contains(e.csv, "1,0,1000,1000,0,Main St & Side St,0,,,,"));
    // Every line of the file Vistro imported as sheeted, except [Network]'s values (the project has
    // none) and Grade at the external ends (not authored there).
    const auto v3 = file("u0-handwritten-v3.csv");
    std::string section; std::size_t compared = 0, skipped = 0;
    for (const auto& line : lines(v3)) {
        if (line.empty() || line.front() == '[') { section = line.empty() ? section : line; continue; }
        const auto record = line.substr(0, line.find(','));
        const bool external = section == "[Links]" && record == "Grade" && line.find(",1,") != record.size();
        if ((section == "[Network]" && record != "RECORDNAME" && record != "Network Settings" && record != "UTDFVERSION" && record != "Metric") || external) { ++skipped; continue; }
        if (!contains(e.csv, line)) throw std::runtime_error("Missing v3 line: " + line);
        ++compared;
    }
    CHECK(skipped == 6 + 4 && compared > 100);
    const auto after = [&](const std::string& title) { return e.csv.substr(e.csv.find(title)); };
    CHECK(after("[Timeplans]") == v3.substr(v3.find("[Timeplans]"))); // timing sections byte for byte
    // External ends list the out-leg arriving at them; Grade is not authored there.
    CHECK(contains(e.csv, "Up ID,2,1,,,,,,,") && contains(e.csv, "Distance,3,,,300,,,,,") && contains(e.csv, "Median,3,,,2.0,,,,,"));
    CHECK(contains(e.csv, "Time,4,,21.6,,,,,,") && contains(e.csv, "Grade,5,,,,,,,,"));
    for (const auto* code : {"UTDF_LEFT_HAND_GEOMETRIC", "UTDF_WORLD_METRES", "UTDF_HCM_METHOD", "UTDF_NEUTRAL_TIMING"}) CHECK(has(e.report, code));
    CHECK(has(e.report, "UTDF_MEDIAN_STEP", "INTID 1 EB leg: set a 2.0 m median") && has(e.report, "UTDF_MEDIAN_STEP", "INTID 1 WB leg"));
    CHECK(!has(e.report, "UTDF_MEDIAN_STEP", " NB leg") && !has(e.report, "UTDF_HCM_DEFAULT"));
}
TEST(utdf, ut1_corridor_numbers_distances_and_external_ends_by_hand) {
    const auto e = utdfDocument(corridor());
    // External ends above 20, in the order of their keying Link ids: "exit" (out-leg's last Link) 21, "up" 22.
    for (const auto* line : {"10,3,0,0,0,A,,,,,", "20,3,300,0,0,B,,,,,", "21,1,400,0,0,,,,,,", "22,1,-200,0,0,,,,,,",
                             "Up ID,10,,,22,,,,,", "Up ID,20,,,10,,,,,", "Up ID,21,,,20,,,,,",
                             "Distance,10,,,200,,,,,", "Distance,20,,,300,,,,,", "Distance,21,,,100,,,,,",
                             "Time,10,,,14.4,,,,,", "Lanes,20,,,1,,,,,", "Speed,21,,,50,,,,,",
                             "Up Node,10,,,,,,,,22,,,,,,", "Dest Node,10,,,,,,,,20,,,,,,", "Dest Node,20,,,,,,,,21,,,,,,",
                             "Lanes,10,,,,,,,,1,,,,,,", "Shared,20,,,,,,,,0,,,,,,", "Width,10,,,,,,,,3.5,,,,,,"})
        if (!contains(e.csv, line)) throw std::runtime_error(std::string("Missing line: ") + line);
}
TEST(utdf, ut2_reexport_is_byte_identical_and_a_new_node_keeps_numbers) {
    const auto d = corridor(); const auto first = utdfDocument(d);
    CHECK(utdfDocument(d).csv == first.csv && utdfDocument(d).report == first.report);
    auto more = d; more.network.nodes.push_back({"c", "C", {{1000, 1000}, {1060, 1000}, {1060, 1060}, {1000, 1060}}, false});
    more.network.nodes.back().number = 30;
    const auto second = utdfDocument(more);
    CHECK(second.csv == first.csv && has(second.report, "UTDF_NO_DETAIL"));
}
TEST(utdf, ut3_each_refusal_names_its_node_and_writes_nothing_for_it) {
    auto unnumbered = corridor(); unnumbered.network.nodes[0].number.reset();
    auto e = utdfDocument(unnumbered);
    CHECK(has(e.report, "UTDF_NODE_UNNUMBERED") && e.report.front().node == "a");
    for (const auto& line : lines(e.csv))
        if (line.rfind("10,", 0) == 0 || (line.find(",10,") == line.find(',') && line.find(',') != std::string::npos)) throw std::runtime_error("Row for 10: " + line);
    CHECK(contains(e.csv, "Up ID,20,,,21,,,,,") && contains(e.csv, "21,1,10,0,0,,,,,,")); // B's leg now starts at an external end
    auto ambiguous = u0Junction(); auto& n = ambiguous.network.nodes[0];
    for (const auto& a : approachLayout(ambiguous.network, n).approaches) n.approachDirections[a.inLink] = ApproachDirection::north;
    e = utdfDocument(ambiguous);
    CHECK(e.csv.empty() && has(e.report, "UTDF_LAYOUT_INVALID", "JUNCTION_DIRECTION_AMBIGUOUS") && has(e.report, "UTDF_NOTHING_TO_EXPORT"));
    auto bare = corridor(); bare.network.nodes.push_back({"c", "C", {{1000, 1000}, {1060, 1000}, {1060, 1060}, {1000, 1060}}, false});
    bare.network.nodes.back().number = 30;
    e = utdfDocument(bare);
    CHECK(has(e.report, "UTDF_NO_DETAIL") && std::find_if(e.report.begin(), e.report.end(), [](const auto& x) { return x.code == "UTDF_NO_DETAIL"; })->node == "c");
    CHECK(!contains(e.csv, "30,3,1030,1030,0,C,,,,,") && e.csv.find("\r\n30,") == std::string::npos);
}
TEST(utdf, ut4_every_empty_authored_field_is_listed) {
    const auto e = utdfDocument(corridor());
    for (const auto* text : {"[Network] DefWidth", "[Network] DefFlow", "[Nodes] CBD at INTID 10", "[Links] Name EB at INTID 10",
                             "[Links] Grade EB at INTID 20", "[Links] Median EB at INTID 21", "[Lanes] Grade EBT at INTID 10"})
        if (!has(e.report, "UTDF_FIELD_EMPTY", text)) throw std::runtime_error(std::string("Not listed: ") + text);
    CHECK(!has(e.report, "UTDF_FIELD_EMPTY", " NB at") && !has(e.report, "UTDF_FIELD_EMPTY", "[Links] Grade EB at INTID 21"));
    const auto u0 = utdfDocument(u0Junction());
    CHECK(!has(u0.report, "UTDF_FIELD_EMPTY", "[Links]") && !has(u0.report, "UTDF_FIELD_EMPTY", "[Lanes]"));
}
TEST(utdf, ut5_export_changes_no_byte_of_the_document) {
    for (const auto& d : {u0Junction(), corridor()}) for (const auto mode : {UtdfMode::geometric, UtdfMode::mirrored}) {
        const auto before = documentJson(d); (void)utdfDocument(d, mode); CHECK(documentJson(d) == before);
    }
}
TEST(utdf, ut11_mirrored_swaps_east_west_and_puts_the_far_turn_under_l) {
    const auto e = utdfDocument(u0Junction(), UtdfMode::mirrored);
    CHECK(contains(e.csv, "1,0,-1000,1000,0,Main St & Side St,0,,,,") && contains(e.csv, "3,1,-1300,1000,0,,,,,,"));
    CHECK(contains(e.csv, "Up ID,1,4,2,3,5,,,,"));
    CHECK(contains(e.csv, "Storage,1,,,40,,,40,60,,,60,,,,")); // kerb pockets NBR/SBR, far pockets EBL/WBL
    CHECK(contains(e.csv, "Shared,1,,1,0,,1,0,0,2,,0,2,,,"));
    CHECK(contains(e.csv, "Phase1,1,,8,,,4,,5,2,,1,6,,,") && contains(e.csv, "PermPhase1,1,,,8,,,4,,,,,,,,")); // groups kept
    CHECK(e.csv.substr(e.csv.find("[Timeplans]")) == utdfDocument(u0Junction()).csv.substr(utdfDocument(u0Junction()).csv.find("[Timeplans]")));
    CHECK(has(e.report, "UTDF_MIRRORED") && !has(e.report, "UTDF_LEFT_HAND_GEOMETRIC"));
}
TEST(utdf, ut0_golden_file_holds_every_name_the_writer_writes) {
    const auto golden = sections(file("u0-vistro.csv"));
    for (const auto* name : {"[Network]", "[Nodes]", "[Links]", "[Lanes]", "[Timeplans]", "[Phases]"}) CHECK(golden.count(name));
    for (const auto& d : {u0Junction(), corridor()}) {
        const auto written = sections(utdfDocument(d).csv);
        CHECK(written.size() == 6);
        for (const auto& [section, value] : written) {
            const auto& g = golden.at(section);
            for (const auto& column : value.first)
                if (std::find(g.first.begin(), g.first.end(), column) == g.first.end()) throw std::runtime_error("Column not in golden file: " + section + " " + column);
            for (const auto& record : value.second)
                if (section != "[Nodes]" && !g.second.count(record)) throw std::runtime_error("Record not in golden file: " + section + " " + record);
        }
    }
    for (const auto* record : {"Control Type", "Cycle Length", "Reference Phase", "Offset"}) CHECK(golden.at("[Timeplans]").second.count(record));
    for (const auto* record : {"BRP", "MinGreen", "MaxGreen", "Yellow", "AllRed", "Recall", "Start", "End", "Yield"}) CHECK(golden.at("[Phases]").second.count(record));
}
TEST(utdf, node_number_is_schema_65_positive_and_unique) {
    const auto d = corridor(); const auto j = documentJson(d);
    CHECK(j["schemaVersion"] == 65 && j["network"]["nodes"][0]["number"] == 10 && kLatestSchema == 65);
    CHECK(documentJson(parseDocument(j)) == j && documentSchemaVersion(parseDocument(j)) == 65);
    auto plain = d; for (auto& n : plain.network.nodes) n.number.reset();
    CHECK(documentSchemaVersion(plain) < 65 && !documentJson(plain)["network"]["nodes"][0].contains("number"));
    auto earlier = j; earlier["schemaVersion"] = 64;
    test::throws([&] { (void)parseDocument(earlier); }, "network.nodes[0].number");
    for (const auto& bad : {Json(0), Json(-3), Json(1.5), Json("1"), Json(nullptr)}) {
        auto x = j; x["network"]["nodes"][0]["number"] = bad;
        test::throws([&] { (void)parseDocument(x); }, "network.nodes[0].number");
    }
    auto twice = corridor(); twice.network.nodes[1].number = 10;
    test::throws([&] { validateDocument(twice); }, "NODE_NUMBER_DUPLICATE");
}
TEST(utdf, cli_fixture_and_expected_files_are_the_writers_output) {
    // tests/utdf_cli.cmake runs multra-cli on this fixture and compares its files byte for byte.
    const auto d = u0Junction(false);
    validateDocument(d);
    CHECK(Json::parse(file("u2-u0-junction.traffic.json")) == documentJson(d));
    CHECK(file("u2-u0-junction-geometric.csv") == utdfDocument(d).csv);
    CHECK(file("u2-u0-junction-mirrored.csv") == utdfDocument(d, UtdfMode::mirrored).csv);
}
