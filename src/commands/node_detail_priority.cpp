#include "node_detail_priority.hpp"
#include "right_of_way_commands.hpp"
#include <algorithm>
#include <set>
#include <stdexcept>
namespace multra {
template<class T> std::set<std::string> idsOf(const std::vector<T>& list) {
    std::set<std::string> ids;
    for (const auto& x : list) ids.insert(x.id);
    return ids;
}
template<class T> void markNew(std::vector<T>& list, const std::set<std::string>& before) {
    for (auto& x : list) if (!before.contains(x.id)) x.generated = true;
}
// S4.5b (§13.4): who gives way between the generated turns, by HCM rank.
// At a signal (S4.5c) `stages` holds the stages the turn is green in; two turns decide only when green together.
// `order` is the turn's place in drawing order, the tie-break a merge's areas share.

bool greenTogether(const GeneratedTurn& x, const GeneratedTurn& y) {
    if (!x.signal) return true;
    return std::any_of(x.stages.begin(), x.stages.end(), [&](auto s) { return std::find(y.stages.begin(), y.stages.end(), s) != y.stages.end(); });
}
void authorNodePriority(ProjectDocument& d, const std::map<std::string, GeneratedTurn>& turns, const PriorityDefaults& defaults) {
    auto& row = d.network.rightOfWay;
    const auto lines = idsOf(row.waitingLines), areas = idsOf(row.conflictAreas), rules = idsOf(row.priorityRules), stops = idsOf(row.stopControls);
    const auto turnOf = [&](const ConflictSide& s) -> const GeneratedTurn* {
        const auto t = turns.find(s.path.connectorId);
        return t == turns.end() ? nullptr : &t->second;
    };
    const auto decides = [&](const ConflictSide& a, const ConflictSide& b) {
        const auto *x = turnOf(a), *y = turnOf(b);
        return x && y && x->inArm != y->inArm && x->rank != y->rank && greenTogether(*x, *y);
    };
    for (const auto& conflict : automaticConflicts(d.network)) {
        if (conflict.kind == ConflictKind::branching || !decides(conflict.first, conflict.second)) continue;
        try { detail::authorCurrentConflict(d, conflict, defaults); }
        catch (const std::invalid_argument& e) { // a merge already taken over whole by an earlier pair
            const std::string code = e.what();
            if (code != "EDIT_NO_MERGE" && code != "EDIT_NO_CROSSING" && code != "EDIT_ALREADY_EXPLICIT") throw;
        }
    }
    std::vector<std::string> stopping;
    const auto added = row.conflictAreas; // setConflictControl edits the list
    for (const auto& a : added) {
        if (areas.contains(a.id) || !turnOf(a.first) || !turnOf(a.second)) continue;
        // A merge is taken over whole, so its pairs need one order or they can cycle: rank, then
        // drawing order, for every pair of it. A crossing was authored only where ranks decide.
        const bool merge = a.kind == ConflictKind::merge;
        if (!merge && !decides(a.first, a.second)) continue;
        const auto key = [&](const ConflictSide& side) { return std::pair{turnOf(side)->rank, turnOf(side)->order}; };
        const bool firstYields = key(a.first) > key(a.second);
        setConflictControl(d, a.id, a.name, firstYields ? ConflictPriority::firstYields : ConflictPriority::secondYields,
                           defaults.gapTime, defaults.headway);
        if (decides(a.first, a.second) && (firstYields ? turnOf(a.first) : turnOf(a.second))->minor) stopping.push_back(a.id);
    }
    for (const auto& id : stopping) setAreaControl(d, id, StopMode::stop);
    markNew(row.waitingLines, lines); markNew(row.conflictAreas, areas); markNew(row.priorityRules, rules); markNew(row.stopControls, stops);
}
}
