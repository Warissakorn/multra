#pragma once
// M6's pre-registered sensitivity sweep (docs/reference/M6_SWEEP.md): phase 1's data levers (D545) and
// phase 2's engine flags (D546), applied to every compiled behaviour after compileDocument. A sweep figure
// decides nothing.
#include "../src/core/types.hpp"
#include <cstring>
#include <optional>
#include <stdexcept>
#include <string>

namespace multra::m6sweep {
struct Candidate { std::string name; std::optional<double> standstill, followingTime; bool standingTarget{}, stopArrival{}; };
inline Candidate candidate(const std::string& name) {
    if (name == "base") return {name, {}, {}};
    if (name == "B") return {name, 0.95, {}};
    if (name == "C") return {name, {}, 1.0};
    if (name == "D") return {name, {}, {}, true, false};  // phase 2 (D546, M6_SWEEP §4): engine levers
    if (name == "E") return {name, {}, {}, false, true};
    if (name == "DE") return {name, {}, {}, true, true};
    throw std::invalid_argument("Unknown candidate: " + name + " (base, B, C, D, E or DE)");
}
inline void apply(Scenario& s, const Candidate& c) {
    for (auto& b : s.behaviours) {
        if (b.w74) throw std::invalid_argument("Candidate levers need the prototype behaviour, not w74: " + b.id);
        if (c.standstill) b.standstillDistance = *c.standstill;
        if (c.followingTime) b.followingTime = *c.followingTime;
        b.standingTarget = c.standingTarget; b.stopArrival = c.stopArrival;
    }
}
// Takes a trailing "--candidate NAME" off argv; nothing when it is absent (outputs then unchanged).
inline std::optional<Candidate> take(int& argc, char** argv) {
    if (argc >= 3 && std::strcmp(argv[argc - 2], "--candidate") == 0) {
        argc -= 2;
        return candidate(argv[argc + 1]);
    }
    return std::nullopt;
}
}
