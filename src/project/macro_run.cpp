#include "macro_run.hpp"
#include "input_manifest.hpp"
#include "macro_settings.hpp"
#include "../core/validate.hpp"
#include <nlohmann/json.hpp>

namespace trafficsim {
std::string macroPeriodName(const MacroPeriod& p) { return p.periodId.empty() ? "matrix:" + p.matrixId : p.periodId; }
MacroRun runMacro(const ProjectDocument& d, const std::filesystem::path& data, const std::string& period, InputManifest* manifest) {
    const auto periods = macroPeriods(d);
    const auto chosen = std::find_if(periods.begin(), periods.end(), [&](const auto& p) { return macroPeriodName(p) == period; });
    if (chosen == periods.end()) throw ValidationError({{"MACRO_PERIOD_UNKNOWN", period}});
    MacroRun run;
    run.period = period; run.revision = d.revision; run.settings = macroSettingsOf(d);
    auto network = macroNetwork(d, data, manifest);
    if (!network.issues.empty()) throw ValidationError(std::move(network.issues));
    auto demand = macroDemand(d, data, *chosen, network.graph, manifest);
    if (!demand.issues.empty()) throw ValidationError(std::move(demand.issues));
    if (manifest) { manifest->validate(); run.inputManifest = manifest->json(); }
    run.graph = std::move(network.graph);
    run.classes = std::move(demand.classes);
    run.advisories = std::move(demand.advisories);
    run.assignment = assignMacro(run.graph, run.classes, run.settings);
    if (!run.assignment.converged) run.advisories.push_back({"MACRO_NOT_CONVERGED", "definition.macroSettings.maxIterations"});
    return run;
}
}
