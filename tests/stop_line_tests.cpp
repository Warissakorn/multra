#include "t_junction_support.hpp"
#include "../src/commands/connector_commands.hpp"
#include "../src/commands/history.hpp"
#include "../src/model/network/conflict_display.hpp"
#include "../src/project/document.hpp"
#include "../src/project/latest_schema.hpp"
#include "../src/project/network_fields.hpp"
using namespace multra;
using namespace tjunction;
// M3.2.12 slices 1b-2, rows SL1-SL3, SL5, SL6 and SL8 of docs/reference/STOP_LINES.md (D594): a side
// that names no line waits at its derived line, frontStopDistance short of its entry.
namespace {
const ConflictArea& areaOf(const ProjectDocument& d, const std::string& id) {
    for (const auto& a : d.network.rightOfWay.conflictAreas) if (a.id == id) return a;
    throw std::runtime_error("no area " + id);
}
double mouthFrom(const Network& n, const ConflictArea& a, bool first) {
    const auto g = conflictAreaGeometry(n, a.kind, a.first, a.second);
    const auto& spans = first ? g.first : g.second;
    if (spans.empty()) throw std::runtime_error("no overlap");
    double from = spans.front().entryStation;
    for (const auto& s : spans) from = std::min(from, s.entryStation);
    return from;
}
// The same document with every derived line stored where it stands: what an older file held.
ProjectDocument stored(ProjectDocument d) { storeDerivedLines(d); return d; }
}
TEST(stoplines, sl1_sl2_derived_merge_lines_stand_before_the_measured_mouth) {
    for (const auto side : {DrivingSide::right, DrivingSide::left}) {
        fixture::TJunctionOptions o; o.side = side;
        const auto t = fixture::tJunction(o);
        const auto& n = t.document.network;
        for (const auto* id : {&t.crossingMerge, &t.nearMerge}) {
            const auto& a = areaOf(t.document, *id);
            for (const bool first : {true, false}) {
                const auto& s = first ? a.first : a.second;
                // The major side stores nothing; the crossing turn's merge is covered by the
                // crossing's sign upstream (§2), the near turn's merge carries its own sign.
                if (s.path.connectorId.empty()) CHECK(s.waitingLineId.empty());
                else if (*id == t.crossingMerge) CHECK(s.waitingLineId == areaOf(t.document, t.crossingArea).first.waitingLineId);
                CHECK(waitingStation(n, s) <= std::max(0.0, mouthFrom(n, a, first) - 1) + 1e-6);
            }
        }
        // The minor road's signs are the author's own lines: still stored.
        CHECK(!areaOf(t.document, t.crossingArea).first.waitingLineId.empty());
        CHECK(!areaOf(t.document, t.nearMerge).first.waitingLineId.empty() || !areaOf(t.document, t.nearMerge).second.waitingLineId.empty());
    }
}
namespace {
fixture::TJunction unsigned_() { fixture::TJunctionOptions o; o.control = std::nullopt; return fixture::tJunction(o); }
}
TEST(stoplines, sl3_a_derived_line_follows_a_reshape_and_the_file_keeps_no_line_for_it) {
    auto t = unsigned_(); // no sign: every line derived
    auto& d = t.document;
    const auto data = test::root() / "data";
    const auto zone = [&](const ProjectDocument& doc) {
        const auto s = compileDocument(doc, data).scenario;
        for (const auto& z : s.conflictZones) if (z.id == "right-of-way/" + t.crossingMerge) return z;
        throw std::runtime_error("no zone");
    };
    const auto before = zone(d);
    const auto lines = d.network.rightOfWay.waitingLines;
    auto geometry = editableConnector(d, t.crossingTurn).geometry;
    geometry[geometry.size() / 2].x -= 2; // bend the turn, as the editor does: one History step (D86 follows)
    History h; h.reset(d);
    CHECK(h.execute("bend", [&](ProjectDocument& x) { changeConnectorGeometry(x, t.crossingTurn, geometry); }));
    d = h.document();
    // No line is added for the derived side; the sign's own line may follow its area (D86/D63).
    CHECK(d.network.rightOfWay.waitingLines.size() == lines.size());
    for (std::size_t k = 0; k < lines.size(); ++k) CHECK(d.network.rightOfWay.waitingLines[k].id == lines[k].id);
    CHECK(areaOf(d, t.crossingMerge).first.waitingLineId.empty() || areaOf(d, t.crossingMerge).second.waitingLineId.empty());
    CHECK(std::abs(zone(d).waitPosition - before.waitPosition) > 0.01); // the forcing: it moved
}
TEST(stoplines, sl5_deriving_keeps_signs_counters_and_dragged_lines_and_resaves_stably) {
    const auto t = fixture::tJunction();
    auto legacy = stored(t.document);
    const auto& row = legacy.network.rightOfWay;
    const std::size_t signs = t.document.network.rightOfWay.waitingLines.size();
    CHECK(row.waitingLines.size() > signs); // the forcing: lines to drop
    // Drag one stored line 0.5 m: it is the author's now.
    auto dragged = row.waitingLines.back(); dragged.point.station -= 0.5; putWaitingLine(legacy, dragged);
    const auto dropped = deriveStopLines(legacy);
    CHECK(dropped == row.waitingLines.size() + dropped - signs - 1);
    CHECK(legacy.network.rightOfWay.waitingLines.size() == signs + 1);
    CHECK(std::any_of(row.waitingLines.begin(), row.waitingLines.end(), [&](const auto& w) { return w.id == dragged.id; }));
    for (const auto& c : row.stopControls)
        CHECK(std::any_of(row.waitingLines.begin(), row.waitingLines.end(), [&](const auto& w) { return w.id == c.waitingLineId; }));
    validateDocument(legacy);
    const auto json = documentJson(legacy);
    CHECK(json.at("schemaVersion") == networkCodec::derivedLineVersion); // D594's step, not the newest
    CHECK(documentJson(parseDocument(json)) == json);
    CHECK(deriveStopLines(legacy) == 0); // a second pass changes nothing
}
TEST(stoplines, sl6_derived_and_stored_at_the_same_places_run_identically) {
    const auto t = fixture::tJunction();
    const auto legacy = stored(t.document);
    CHECK(legacy.network.rightOfWay.waitingLines.size() > t.document.network.rightOfWay.waitingLines.size());
    const auto data = test::root() / "data";
    const auto a = compileDocument(t.document, data).scenario, b = compileDocument(legacy, data).scenario;
    CHECK(a.conflictZones == b.conflictZones);
    const auto ra = runSimulation(a, 42), rb = runSimulation(b, 42);
    CHECK(ra.completed == rb.completed && ra.completed > 0);
    CHECK(run(t.document, t).report.movements == run(legacy, t).report.movements);
}
TEST(stoplines, sl8_front_stop_distance_is_a_project_value_in_0_to_10_metres) {
    auto t = unsigned_();
    auto& d = t.document;
    const auto data = test::root() / "data";
    const auto wait = [&] {
        // Named, not iterated off a member of the temporary snapshot: the MSVC run found no zone.
        const auto s = compileDocument(d, data).scenario;
        for (const auto& z : s.conflictZones)
            if (z.id == "right-of-way/" + t.crossingMerge) return z.waitPosition;
        throw std::runtime_error("no zone");
    };
    const double base = wait();
    d.network.rightOfWay.frontStopDistance = 2;
    validateDocument(d);
    test::near(wait(), base - 1, 1e-6); // one metre further back
    const auto json = documentJson(d);
    CHECK(json.at("network").at("rightOfWay").at("frontStopDistance") == 2.0);
    CHECK(parseDocument(json).network.rightOfWay.frontStopDistance == 2.0);
    for (const double bad : {0.0, -1.0, 10.5, std::nan("")}) {
        auto j = json; j["network"]["rightOfWay"]["frontStopDistance"] = bad;
        bool refused = false;
        try { parseDocument(j); } catch (const ValidationError& e) {
            refused = std::any_of(e.issues.begin(), e.issues.end(), [](const auto& i) {
                return i.code == "INVALID_FRONT_STOP_DISTANCE" && i.path == "rightOfWay.frontStopDistance"; });
        } catch (const std::exception&) { refused = std::isnan(bad); } // NaN is not JSON: refused as a number
        CHECK(refused);
    }
    // Older schemas know neither the value nor an empty line.
    auto old = json; old["schemaVersion"] = networkCodec::derivedLineVersion - 1;
    test::throws([&] { parseDocument(old); }, "EDIT_UNSUPPORTED_FIELD");
    old["network"]["rightOfWay"].erase("frontStopDistance");
    test::throws([&] { parseDocument(old); }, "UNKNOWN_WAITING_LINE");
}
TEST(stoplines, sl4_a_placed_line_covers_the_areas_downstream_of_it) {
    const auto t = unsigned_();
    const auto& n = t.document.network;
    const double end = polylineLength(std::find_if(n.links.begin(), n.links.end(), [&](const auto& l) { return l.id == t.minor; })->geometry);
    const ControlPathRef approach{t.minor, n.links[2].lanes.front().id, "", "", ""};
    CHECK(n.links[2].id == t.minor);
    // On the minor approach, 2 m before its end: every area either turn gives way at.
    auto d = t.document;
    const auto line = placeStopLine(d, {approach, end - 2});
    for (const auto* id : {&t.crossingArea, &t.crossingMerge, &t.nearMerge}) {
        const auto& a = areaOf(d, *id);
        CHECK((a.priority == ConflictPriority::firstYields ? a.first : a.second).waitingLineId == line);
    }
    validateDocument(d);
    const auto r = resolveRightOfWay(d.network, runtimeSections(d.network), {5, 7});
    CHECK(r.issues.empty());
    for (const auto& z : r.zones) CHECK(z.waitPosition < 0); // every minor side waits on the approach, before its Connector
    // A second line upstream covers nothing the first already does.
    test::throws([&] { auto again = d; placeStopLine(again, {approach, end - 12}); }, "EDIT_NO_CONFLICT_DOWNSTREAM");
    // On the crossing turn before its crossing: that turn's areas only.
    auto turn = t.document;
    const auto& crossing = areaOf(turn, t.crossingArea);
    const auto& minorSide = crossing.first.path.connectorId == t.crossingTurn ? crossing.first : crossing.second;
    const auto onTurn = placeStopLine(turn, {minorSide.path, minorSide.entryStation - 3});
    CHECK(stopLineCoverage(t.document.network, {minorSide.path, minorSide.entryStation - 3}).size() == 2);
    const auto& near = areaOf(turn, t.nearMerge);
    CHECK((near.priority == ConflictPriority::firstYields ? near.first : near.second).waitingLineId.empty());
    // Removing it hands the areas back to their derived lines: the drawing is as it was.
    removeStopLine(turn, onTurn);
    CHECK(turn.network == t.document.network); // only the id counter moved on
    test::throws([&] { removeStopLine(turn, onTurn); }, "EDIT_UNKNOWN_OBJECT");
}
