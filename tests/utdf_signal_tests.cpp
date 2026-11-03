// UTDF U2a (UTDF_EXPORT §7 U2a contract, D443, §10 UT7/UT8/UT11): timing from generated NEMA
// controllers on both driving sides, and a controller that fails nemaCheck.
#include "junction_signal_fixture.hpp"
#include "../src/project/utdf_export.hpp"
#include <sstream>
using namespace multra;
namespace {
std::map<std::string, std::vector<std::string>> section(const std::string& csv, const std::string& title, int intid) {
    std::map<std::string, std::vector<std::string>> rows; std::istringstream s(csv.substr(csv.find(title))); std::string line;
    std::getline(s, line);
    while (std::getline(s, line) && line != "\r") {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        std::vector<std::string> cells; std::string cell; std::istringstream c(line);
        while (std::getline(c, cell, ',')) cells.push_back(cell);
        if (line.back() == ',') cells.push_back("");
        if (cells.size() > 2 && cells[1] == std::to_string(intid)) rows[cells[0]] = {cells.begin() + 2, cells.end()};
    }
    return rows;
}
double value(const std::string& s) { return std::stod(s); }
bool has(const std::vector<UtdfNote>& report, const std::string& code, const std::string& text = {}) {
    return std::any_of(report.begin(), report.end(), [&](const auto& n) { return n.code == code && n.text.find(text) != std::string::npos; });
}
}
TEST(utdf, ut7_each_preset_writes_the_generated_controllers_timing) {
    for (const auto side : {DrivingSide::left, DrivingSide::right}) for (const auto* preset : {"P1", "P2", "P3", "P4"}) {
        History h; h.reset(junctionSignalTest::signalCross(side, preset)); junctionTest::apply(h);
        auto d = h.document(); d.network.nodes[0].number = 7;
        const auto& c = d.definition->signalControllers.at(0);
        CHECK(nemaCheck(c, {d.network, approachLayout(d.network, d.network.nodes[0])}).empty()); // forcing
        const auto e = utdfDocument(d);
        CHECK(e.csv.find("\r\n7,0,") != std::string::npos);
        const auto phases = section(e.csv, "[Phases]", 7);
        CHECK(section(e.csv, "[Timeplans]", 7).at("Cycle Length")[0] == std::to_string(static_cast<int>(c.cycle)) + ".0");
        std::map<int, double> ring;
        for (const auto& p : junctionSignalTest::timing(preset).phases) {
            const auto i = static_cast<std::size_t>(p.number - 1);
            test::near(value(phases.at("MinGreen")[i]), *p.green, 0.05);
            test::near(value(phases.at("Yellow")[i]), *p.yellow, 0.05);
            CHECK(value(phases.at("AllRed")[i]) >= *p.allRed - 0.05); // waiting time counts as all-red (D443)
            test::near(value(phases.at("MinGreen")[i]) + value(phases.at("Yellow")[i]) + value(phases.at("AllRed")[i]), value(phases.at("MinSplit")[i]), 0.15);
            ring[(p.number - 1) / 4] += value(phases.at("MinSplit")[i]);
        }
        for (const auto& [r, total] : ring) { (void)r; test::near(total, c.cycle, 0.3); }
        const auto lanes = section(e.csv, "[Lanes]", 7);
        std::set<std::string> numbers(lanes.at("Phase1").begin(), lanes.at("Phase1").end());
        for (const auto* n : {"2", "4", "6", "8"}) CHECK(numbers.count(n));
        for (const auto& [record, cells] : phases) { (void)cells; CHECK(record != "StartTime"); }
    }
}
TEST(utdf, ut8_a_controller_failing_nema_is_left_out_unless_written_unsignalised) {
    std::ifstream f(test::root() / "tests/node-analysis-four-leg.traffic.json"); Json j; f >> j;
    auto d = parseDocument(j); d.network.nodes[0].number = 3;
    auto e = utdfDocument(d);
    CHECK(e.csv.empty() && has(e.report, "UTDF_NEMA_INVALID", "NEMA_PHASE_NUMBER") && has(e.report, "UTDF_NOTHING_TO_EXPORT"));
    e = utdfDocument(d, UtdfOptions{UtdfMode::geometric, true, {}});
    CHECK(has(e.report, "UTDF_NEMA_UNSIGNALISED") && e.csv.find("\r\n3,3,") != std::string::npos);
    const auto lanes = section(e.csv, "[Lanes]", 3);
    CHECK(lanes.count("Volume") && !lanes.count("Phase1") && !lanes.count("PermPhase1"));
    CHECK(e.csv.find("RECORDNAME,INTID,DATA\r\n\r\n[Phases]") != std::string::npos); // no timing for it
}
TEST(utdf, ut11_mirrored_keeps_group_numbers_with_the_far_turn_under_l) {
    for (const auto side : {DrivingSide::left, DrivingSide::right}) {
        History h; h.reset(junctionSignalTest::signalCross(side, "P3")); junctionTest::apply(h);
        auto d = h.document(); d.network.nodes[0].number = 7;
        const auto geometric = section(utdfDocument(d).csv, "[Lanes]", 7), mirrored = section(utdfDocument(d, UtdfMode::mirrored).csv, "[Lanes]", 7);
        CHECK(section(utdfDocument(d).csv, "[Phases]", 7) == section(utdfDocument(d, UtdfMode::mirrored).csv, "[Phases]", 7));
        // Columns NBL..WBR: the far turn is R (left-hand) or L (right-hand) when geometric; mirrored puts
        // left-hand's far turn under L, so its protected odd phase moves from R to L.
        const auto oddAt = [](const std::vector<std::string>& phase1, char movement) {
            for (std::size_t i = 0; i < 12; ++i) if ("LTR"[i % 3] == movement && !phase1[i].empty() && std::stoi(phase1[i]) % 2 == 1) return true;
            return false;
        };
        const char far = side == DrivingSide::left ? 'R' : 'L', mirroredFar = far == 'R' ? 'L' : 'R';
        CHECK(oddAt(geometric.at("Phase1"), far) && !oddAt(geometric.at("Phase1"), mirroredFar));
        CHECK(oddAt(mirrored.at("Phase1"), mirroredFar) && !oddAt(mirrored.at("Phase1"), far));
    }
}
TEST(utdf, number_nodes_is_one_undo_step_and_apply_keeps_a_number) {
    History h; h.reset(junctionSignalTest::signalCross(DrivingSide::left, "P2"));
    CHECK(!junctionTest::node(h.document()).number);
    junctionTest::apply(h); CHECK(junctionTest::node(h.document()).number == 1); // Apply numbers it
    auto d = h.document(); setNodeNumber(d, junctionTest::node(d).id, 9);
    (void)generateJunction(d, junctionTest::node(d).id, junctionTest::catalogs(d));
    CHECK(junctionTest::node(d).number == 9); // re-Apply keeps it
    d.network.nodes.push_back({"b", "B", {{500, 500}, {560, 500}, {560, 560}, {500, 560}}, true});
    d.network.nodes.push_back({"c", "C", {{700, 500}, {760, 500}, {760, 560}, {700, 560}}, false});
    History n; n.reset(d); const auto states = n.states().size(); std::vector<std::string> ids;
    CHECK(n.execute("editorNumberNodes", [&](auto& doc) { ids = numberNodes(doc); }));
    CHECK(n.states().size() == states + 1 && ids == std::vector<std::string>({"b", "c"}));
    const auto& nodes = n.document().network.nodes;
    CHECK(nodes[1].number == 10 && nodes[2].number == 11 && nodes[1].generated); // a number leaves `generated` alone
    CHECK(numberNodes(*std::make_unique<ProjectDocument>(n.document())).empty());
    auto twice = n.document(); setNodeNumber(twice, "c", 9);
    test::throws([&] { validateDocument(twice); }, "NODE_NUMBER_DUPLICATE");
    n.undo(); CHECK(!n.document().network.nodes[1].number);
}
