#include "utdf_export.hpp"
#include "utdf_legs.hpp"
#include "document.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <iomanip>
#include <locale>
#include <map>
#include <set>
#include <sstream>

namespace multra {
namespace {
using utdf::Plan; using utdf::WrittenNode;
// Lengths 0.1 m, widths 0.01 m, trailing zeros trimmed; `fixed` keeps the decimals (Time, Median).
std::string number(double v, int decimals, bool fixed = false) {
    std::ostringstream s; s.imbue(std::locale::classic());
    s << std::fixed << std::setprecision(decimals) << v;
    auto t = s.str();
    if (!fixed && t.find('.') != std::string::npos) {
        while (t.back() == '0') t.pop_back();
        if (t.back() == '.') t.pop_back();
    }
    if (t.find_first_not_of("-0.") == std::string::npos && t.front() == '-') t.erase(0, 1); // no "-0"
    return t;
}
const std::array<const char*, 8> kLinkColumns{"NB", "SB", "EB", "WB", "NE", "NW", "SE", "SW"};
const std::array<const char*, 4> kApproaches{"NB", "SB", "EB", "WB"};
const char* compass(ApproachDirection d, UtdfMode mode) {
    const bool m = mode == UtdfMode::mirrored;
    switch (d) {
    case ApproachDirection::north: return "NB";
    case ApproachDirection::south: return "SB";
    case ApproachDirection::east: return m ? "WB" : "EB";
    case ApproachDirection::west: return m ? "EB" : "WB";
    }
    return "NB";
}
char letter(JunctionMovement j, UtdfMode mode) {
    if (j == JunctionMovement::through) return 'T';
    return (j == JunctionMovement::left) != (mode == UtdfMode::mirrored) ? 'L' : 'R';
}
std::string text(std::string s, std::vector<UtdfNote>& report, const std::string& node) {
    if (s.find_first_of(",\r\n\"") == std::string::npos) return s;
    for (auto& c : s) if (c == ',' || c == '\r' || c == '\n' || c == '"') c = c == ',' ? ';' : ' ';
    report.push_back({"UTDF_NAME_CHANGED", node, "A name's comma, quote or line break was replaced: " + s});
    return s;
}
const StreetSegment* segmentOf(const Network& n, const Link& l) {
    if (!l.streetSegmentId) return nullptr;
    const auto s = std::find_if(n.streetSegments.begin(), n.streetSegments.end(), [&](const auto& x) { return x.id == *l.streetSegmentId; });
    return s == n.streetSegments.end() ? nullptr : &*s;
}
std::optional<double> median(const ProjectDocument& d, const Link& l) {
    const auto* s = segmentOf(d.network, l);
    if (!s) return {};
    if (s->overrides.median) return s->overrides.median;
    if (!s->crossSectionId || !d.definition) return {};
    const auto& xs = d.definition->crossSections;
    const auto x = std::find_if(xs.begin(), xs.end(), [&](const auto& c) { return c.id == *s->crossSectionId; });
    return x == xs.end() ? std::nullopt : std::optional<double>(x->median);
}
// One section's rows keyed by INTID, then record, with cells by column name.
struct Rows {
    std::vector<std::string> records;
    std::map<int, std::map<std::string, std::map<std::string, std::string>>> cells;
    std::vector<UtdfNote>* report{};
    // An authored cell: written, or empty and listed (UT4).
    void put(int id, const std::string& record, const std::string& column, std::optional<std::string> value, const std::string& node, const char* section) {
        if (!value || value->empty()) {
            report->push_back({"UTDF_FIELD_EMPTY", node, std::string(section) + " " + record + " " + column + " at INTID " + std::to_string(id) + " is empty."});
            value = "";
        }
        cells[id][record][column] = *value;
    }
    std::string csv(const std::vector<std::string>& columns) const {
        std::string out;
        for (const auto& [id, records] : cells) for (const auto& record : this->records) {
            const auto r = records.find(record);
            if (r == records.end()) continue;
            out += record + "," + std::to_string(id);
            for (const auto& c : columns) { out += ","; if (const auto v = r->second.find(c); v != r->second.end()) out += v->second; }
            out += "\r\n";
        }
        return out;
    }
};
std::optional<std::string> optionalNumber(const std::optional<double>& v, int decimals, bool fixed = false) {
    return v ? std::optional<std::string>(number(*v, decimals, fixed)) : std::nullopt;
}
void linkRecords(Rows& rows, const ProjectDocument& d, int id, const std::string& column, int up, double distance, const Link& named, const Link& arriving, std::optional<double> grade, bool authoredGrade, const std::string& node) {
    const auto* segment = segmentOf(d.network, named);
    const auto name = segment && !segment->name.empty() ? segment->name : named.name;
    const auto speed = arriving.speedLimit;
    rows.put(id, "Up ID", column, std::to_string(up), node, "[Links]");
    rows.put(id, "Lanes", column, std::to_string(arriving.lanes.size()), node, "[Links]");
    rows.put(id, "Name", column, text(name, *rows.report, node), node, "[Links]");
    rows.put(id, "Distance", column, number(std::round(distance), 0), node, "[Links]");
    rows.put(id, "Speed", column, optionalNumber(speed, 1), node, "[Links]");
    rows.put(id, "Time", column, speed && *speed > 0 ? std::optional<std::string>(number(distance * 3.6 / *speed, 1, true)) : std::nullopt, node, "[Links]");
    if (authoredGrade) rows.put(id, "Grade", column, optionalNumber(grade, 1), node, "[Links]");
    else rows.cells[id]["Grade"][column] = "";
    rows.put(id, "Median", column, optionalNumber(median(d, named), 1, true), node, "[Links]");
}
std::optional<double> gradeOf(const Node& node, const std::string& link) {
    if (!node.analysis) return {};
    for (const auto& a : node.analysis->approaches) if (a.link == link) return a.grade;
    return {};
}
void laneRecords(Rows& rows, const Plan& p, std::size_t i, UtdfMode mode) {
    const auto& w = p.nodes[i]; const auto& node = w.node->id;
    const auto destination = [&](std::size_t out) {
        if (w.out[out].node) return p.nodes[*w.out[out].node].number;
        return p.outEnd.at({i, out});
    };
    for (std::size_t a = 0; a < w.layout.approaches.size(); ++a) {
        const auto& approach = w.layout.approaches[a];
        const std::string prefix = compass(approach.direction, mode);
        const int up = w.in[a].node ? p.nodes[*w.in[a].node].number : p.inEnd.at({i, a});
        std::map<std::size_t, char> turnLetter; std::vector<char> present;
        for (const auto& m : w.layout.movements) if (w.layout.membership.turns[m.turn].in == a) {
            turnLetter[m.turn] = letter(m.movement, mode); present.push_back(turnLetter[m.turn]);
            const auto column = prefix + turnLetter[m.turn];
            rows.put(w.number, "Up Node", column, std::to_string(up), node, "[Lanes]");
            rows.put(w.number, "Dest Node", column, std::to_string(destination(w.layout.membership.turns[m.turn].out)), node, "[Lanes]");
        }
        // Lane groups (contract): T owns any lane serving T; a lane serving both turns only is L's.
        std::map<char, std::vector<const LayoutLane*>> group, serving;
        std::map<char, bool> sharesL, sharesR;
        for (const auto& lane : approach.lanes) {
            std::set<char> s;
            for (const auto t : lane.turns) if (turnLetter.count(t)) s.insert(turnLetter[t]);
            for (const auto c : s) serving[c].push_back(&lane);
            if (s.empty()) continue;
            const char g = s.count('T') ? 'T' : s.count('L') ? 'L' : 'R';
            group[g].push_back(&lane);
            sharesL[g] = sharesL[g] || (g != 'L' && s.count('L'));
            sharesR[g] = sharesR[g] || (g != 'R' && s.count('R'));
        }
        const char first = std::count(present.begin(), present.end(), 'T') ? 'T' : present.empty() ? 'T' : present.front();
        for (const char c : std::string("LTR")) {
            if (!std::count(present.begin(), present.end(), c)) continue;
            const auto column = prefix + c; const auto& lanes = group[c];
            rows.put(w.number, "Lanes", column, std::to_string(lanes.size()), node, "[Lanes]");
            if (!lanes.empty()) rows.put(w.number, "Shared", column, std::to_string((sharesL[c] ? 1 : 0) + (sharesR[c] ? 2 : 0)), node, "[Lanes]");
            const auto& widths = lanes.empty() ? serving[c] : lanes;
            double sum = 0; for (const auto* l : widths) sum += l->width;
            rows.put(w.number, "Width", column, widths.empty() ? std::nullopt : std::optional<std::string>(number(sum / static_cast<double>(widths.size()), 2)), node, "[Lanes]");
            const LayoutLane* pocket = nullptr; int count = 0;
            for (const auto* l : lanes) if (l->pocket) { ++count; if (!pocket || l->pocket->storage > pocket->pocket->storage) pocket = l; }
            if (pocket) {
                rows.put(w.number, "Storage", column, number(pocket->pocket->storage, 1), node, "[Lanes]");
                rows.put(w.number, "Taper", column, number(pocket->pocket->taper, 1), node, "[Lanes]");
                rows.put(w.number, "StLanes", column, std::to_string(count), node, "[Lanes]");
            }
            if (c == first) {
                rows.put(w.number, "Grade", column, optionalNumber(gradeOf(*w.node, approach.inLink), 1), node, "[Lanes]");
                rows.put(w.number, "Speed", column, optionalNumber(w.in[a].links.back()->speedLimit, 1), node, "[Lanes]");
            }
        }
    }
}
}
UtdfExport utdfDocument(const ProjectDocument& d, UtdfMode mode) {
    UtdfExport out;
    const auto p = utdf::plan(d.network);
    out.report = p.refusals;
    if (p.nodes.empty()) {
        out.report.push_back({"UTDF_NOTHING_TO_EXPORT", "", "No Node can be written; nothing was exported."});
        return out;
    }
    const auto x = [&](Point q) { return number(mode == UtdfMode::mirrored ? -q.x : q.x, 1); };
    std::string csv = "[Network]\r\nNetwork Settings\r\nRECORDNAME,DATA\r\nUTDFVERSION,8\r\nMetric,1\r\nDefWidth,\r\nDefFlow,\r\n\r\n";
    out.report.push_back({"UTDF_FIELD_EMPTY", "", "[Network] DefWidth is empty: the project has no network-wide lane width."});
    out.report.push_back({"UTDF_FIELD_EMPTY", "", "[Network] DefFlow is empty: the project has no network-wide saturation flow."});
    // [Nodes]
    std::map<int, std::string> nodes;
    for (const auto& w : p.nodes) {
        const auto type = w.layout.membership.control == SectionControl::signalised ? "0" : "3";
        const auto cbd = w.node->analysis ? std::string(w.node->analysis->cbd ? "1" : "0") : std::string();
        if (cbd.empty()) out.report.push_back({"UTDF_FIELD_EMPTY", w.node->id, "[Nodes] CBD at INTID " + std::to_string(w.number) + " is empty: the Node has no analysis."});
        nodes[w.number] = std::to_string(w.number) + "," + type + "," + x(w.centroid) + "," + number(w.centroid.y, 1) + ",0," +
                          text(w.node->name, out.report, w.node->id) + "," + cbd + ",,,,";
    }
    for (const auto& e : p.ends) nodes[e.number] = std::to_string(e.number) + ",1," + x(e.at) + "," + number(e.at.y, 1) + ",0,,,,,,";
    csv += "[Nodes]\r\nNode Data\r\nINTID,TYPE,X,Y,Z,DESCRIPTION,CBD,Inside Radius,Outside Radius,Roundabout Lanes,Circle Speed\r\n";
    for (const auto& [id, row] : nodes) { (void)id; csv += row + "\r\n"; }
    // [Links]
    Rows links{{"Up ID", "Lanes", "Name", "Distance", "Speed", "Time", "Grade", "Median"}, {}, &out.report};
    for (std::size_t i = 0; i < p.nodes.size(); ++i) {
        const auto& w = p.nodes[i];
        for (std::size_t a = 0; a < w.in.size(); ++a) {
            const auto& walk = w.in[a];
            const int up = walk.node ? p.nodes[*walk.node].number : p.inEnd.at({i, a});
            const Point from = walk.node ? p.nodes[*walk.node].centroid : walk.links.front()->geometry.front();
            linkRecords(links, d, w.number, compass(w.layout.approaches[a].direction, mode), up, utdf::walkLength(from, walk, w.centroid),
                        *walk.links.back(), *walk.links.back(), gradeOf(*w.node, w.layout.approaches[a].inLink), true, w.node->id);
        }
        for (std::size_t k = 0; k < w.out.size(); ++k) if (!w.out[k].node) {
            const auto& walk = w.out[k];
            linkRecords(links, d, p.outEnd.at({i, k}), compass(w.outDirection[k], mode), w.number,
                        utdf::walkLength(w.centroid, walk, walk.links.back()->geometry.back()), *walk.links.front(), *walk.links.back(), {}, false, w.node->id);
        }
    }
    csv += "\r\n[Links]\r\nLink Data\r\nRECORDNAME,INTID,NB,SB,EB,WB,NE,NW,SE,SW\r\n";
    csv += links.csv({kLinkColumns.begin(), kLinkColumns.end()});
    // [Lanes]
    Rows lanes{{"Up Node", "Dest Node", "Lanes", "Shared", "Width", "Storage", "Taper", "StLanes", "Grade", "Speed"}, {}, &out.report};
    for (std::size_t i = 0; i < p.nodes.size(); ++i) laneRecords(lanes, p, i, mode);
    std::vector<std::string> columns;
    for (const auto* a : kApproaches) for (const char c : std::string("LTR")) columns.push_back(std::string(a) + c);
    columns.push_back("PED"); columns.push_back("HOLD");
    csv += "\r\n[Lanes]\r\nLane Group Data\r\nRECORDNAME,INTID";
    for (const auto& c : columns) csv += "," + c;
    csv += "\r\n" + lanes.csv(columns);
    out.csv = std::move(csv);
    out.report.push_back({"UTDF_WORLD_METRES", "", "X and Y are world metres; UTM coordinates come with the desktop export (U2)."});
    out.report.push_back({"UTDF_GEOMETRY_ONLY", "", "Volumes, Timeplans and Phases are not written yet (U2)."});
    if (mode == UtdfMode::mirrored)
        out.report.push_back({"UTDF_MIRRORED", "", "The map is mirrored (x -> -x): E and W, and L and R, are swapped so the far turn is under L."});
    else if (d.network.drivingSide == DrivingSide::left)
        out.report.push_back({"UTDF_LEFT_HAND_GEOMETRIC", "", "The network is left-hand: the far turn is under R. A right-hand reader (Synchro, Vistro after import) reads R as the kerb turn."});
    return out;
}
}
