#pragma once
// M6's pre-registered sensitivity sweep, phase 1 (D545, docs/reference/M6_SWEEP.md): the data-only
// levers, applied to every compiled behaviour after compileDocument. A sweep figure decides nothing.
#include "../src/core/types.hpp"
#include <cstring>
#include <optional>
#include <stdexcept>
#include <string>

namespace multra::m6sweep {
struct Candidate { std::string name; std::optional<double> standstill, followingTime; };
inline Candidate candidate(const std::string& name) {
    if (name == "base") return {name, {}, {}};
    if (name == "B") return {name, 0.95, {}};
    if (name == "C") return {name, {}, 1.0};
    throw std::invalid_argument("Unknown candidate: " + name + " (base, B or C)");
}
inline void apply(Scenario& s, const Candidate& c) {
    for (auto& b : s.behaviours) {
        if (b.w74) throw std::invalid_argument("Candidate levers need the prototype behaviour, not w74: " + b.id);
        if (c.standstill) b.standstillDistance = *c.standstill;
        if (c.followingTime) b.followingTime = *c.followingTime;
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
