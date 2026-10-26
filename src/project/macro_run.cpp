#include "macro_run.hpp"
#include "input_manifest.hpp"
#include "macro_settings.hpp"
#include "../core/validate.hpp"
#include <algorithm>
#include <nlohmann/json.hpp>

namespace multra {
std::string macroPeriodName(const MacroPeriod& p) { return p.periodId.empty() ? "matrix:" + p.matrixId : p.periodId; }
namespace {
MacroRun executeMacro(const ProjectDocument& d, const std::filesystem::path& data, const std::string& period,
                      const MacroSettings& settings, const std::string& settingsPath, InputManifest* manifest,
                      const MacroObserver* observer, bool keepPaths) {
    const auto enter = [&](MacroStage s) { if (observer && observer->stage) observer->stage(s); };
    enter(MacroStage::period);
    if (auto issues = macroSettingsIssues(settings); !issues.empty()) {
        for (auto& issue : issues) issue.path = settingsPath + issue.path.substr(std::string("definition.macroSettings").size());
        throw ValidationError(std::move(issues));
    }
    const auto periods = macroPeriods(d);
    const auto chosen = std::find_if(periods.begin(), periods.end(), [&](const auto& p) { return macroPeriodName(p) == period; });
    if (chosen == periods.end()) throw ValidationError({{"MACRO_PERIOD_UNKNOWN", period}});
    MacroRun run;
    run.period = period; run.revision = d.revision; run.settings = settings;
    enter(MacroStage::graph);
    auto network = macroNetwork(d, data, manifest);
    if (!network.issues.empty()) throw ValidationError(std::move(network.issues));
    enter(MacroStage::demand);
    auto demand = macroDemand(d, data, *chosen, network.graph, manifest);
    if (!demand.issues.empty()) throw ValidationError(std::move(demand.issues));
    if (manifest) { manifest->validate(); run.inputManifest = manifest->json(); }
    run.graph = std::move(network.graph);
    run.classes = std::move(demand.classes);
    run.advisories = std::move(network.advisories); // S3.6a
    run.advisories.insert(run.advisories.end(), demand.advisories.begin(), demand.advisories.end());
    enter(MacroStage::assignment);
    run.assignment = assignMacro(run.graph, run.classes, run.settings, observer ? observer->iteration : std::function<bool(const MacroIteration&)>{}, keepPaths);
    if (!run.assignment.converged && !run.assignment.cancelled) run.advisories.push_back({"MACRO_NOT_CONVERGED", settingsPath + ".maxIterations"});
    return run;
}
}
MacroRun runMacro(const ProjectDocument& d, const std::filesystem::path& data, const std::string& period, InputManifest* manifest,
                  const MacroObserver* observer, bool keepPaths) {
    return executeMacro(d, data, period, macroSettingsOf(d), "definition.macroSettings", manifest, observer, keepPaths);
}
MacroRun runNamedMacro(const ProjectDocument& d, const std::filesystem::path& data, const std::string& id, InputManifest* manifest,
                       const MacroObserver* observer, bool keepPaths) {
    const auto found = std::find_if(d.macroRuns.begin(), d.macroRuns.end(), [&](const auto& run) { return run.id == id; });
    if (found == d.macroRuns.end()) {
        if (observer && observer->stage) observer->stage(MacroStage::period);
        throw ValidationError({{"MACRO_RUN_UNKNOWN", "macroRuns"}});
    }
    const auto index = static_cast<std::size_t>(found - d.macroRuns.begin());
    auto result = executeMacro(d, data, found->period, found->settings, "macroRuns[" + std::to_string(index) + "].settings", manifest, observer, keepPaths);
    result.definitionId = found->id; result.definitionName = found->name;
    return result;
}
}
