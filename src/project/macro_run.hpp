#pragma once
#include "macro_input.hpp"
#include "../macro/assign.hpp"
#include <nlohmann/json.hpp>

namespace trafficsim {
// S2.3 (MACRO_NETWORK §7, §9, D275): one Macro run of a named period, shared by the CLI and the
// editor. A period is named by its study period's id, or `matrix:<id>` for an own-times matrix.
std::string macroPeriodName(const MacroPeriod&);
struct MacroRun {
    std::string period;
    std::uint64_t revision{};
    MacroSettings settings;
    MacroGraph graph;
    std::vector<MacroClass> classes;
    MacroAssignment assignment;
    std::vector<ValidationIssue> advisories; // the demand's, then MACRO_NOT_CONVERGED when stopped at the cap
    Json inputManifest;                      // the files read; null without a manifest
};
// Refused by ValidationError: MACRO_PERIOD_UNKNOWN (`period`), then the graph's and the demand's issues.
MacroRun runMacro(const ProjectDocument&, const std::filesystem::path& dataDirectory, const std::string& period,
                  InputManifest* = nullptr);
}
