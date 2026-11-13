// DYNAMIC_ASSIGNMENT §4, §6 (D549): the MSA loop over real Micro runs.
#include "dynamic_assignment_loop.hpp"
#include "build_info.hpp"
#include "meso_assignment.hpp"
#include "../core/validate.hpp"

namespace multra {
RouteCosts runDynamicAssignment(const ProjectDocument& doc, const std::filesystem::path& data, const DaSettings& settings,
                                std::uint32_t seed, const std::string& projectName, std::ostream& log, DaLevel level) {
    if (!doc.definition) throw ValidationError({{"DYNAMIC_ASSIGNMENT_INVALID", "routingDecisions"}});
    const auto plan = planDynamicAssignment(*doc.definition, level);
    DaMeasurement last;
    const auto result = iterateMsa(decisionWeights(*doc.definition, plan), [&](const std::vector<std::vector<double>>& w) {
        auto run = doc;
        applyDecisionWeights(*run.definition, plan, w);
        last = level == DaLevel::meso ? measureMesoDecisionCosts(run, data, plan) : measureDecisionCosts(run, data, plan, seed);
        return last.cells;
    }, settings);
    for (const auto& it : result.iterations) log << "dynamic assignment: iteration " << it.n << ", delta " << it.delta << '\n';
    if (!result.converged)
        log << "dynamic assignment: not converged after " << result.iterations.size() << " iterations (delta "
            << result.iterations.back().delta << ")\n";
    auto out = routeCostsFor(*doc.definition, plan, result.iterations.back().weights, last,
                             {projectName, seed, settings, std::string(MULTRA_VERSION)});
    out.level = level;
    out.converged = result.converged;
    for (const auto& it : result.iterations) out.deltas.push_back(it.delta);
    return out;
}
}
