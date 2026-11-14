#include "procedure_run.hpp"
#include "input_manifest.hpp"
#include <algorithm>

namespace multra {
const char* procedureStepStatusName(ProcedureStepStatus s) {
    switch (s) {
    case ProcedureStepStatus::done: return "done";
    case ProcedureStepStatus::skipped: return "skipped";
    case ProcedureStepStatus::refused: return "refused";
    case ProcedureStepStatus::notRun: return "notRun";
    }
    return "notRun";
}
bool ProcedureRun::completed() const {
    return !cancelled && std::none_of(steps.begin(), steps.end(), [](const auto& s) {
        return s.status == ProcedureStepStatus::refused || s.status == ProcedureStepStatus::notRun; });
}
ProcedureRun runProcedure(const ProjectDocument& d, const std::filesystem::path& data, const std::string& id,
                          const InputManifest* manifest, const ProcedureObserver* observer) {
    const auto found = std::find_if(d.procedures.begin(), d.procedures.end(), [&](const auto& p) { return p.id == id; });
    if (found == d.procedures.end()) throw ValidationError({{"PROCEDURE_UNKNOWN", "procedures"}});
    const auto at = "procedures[" + std::to_string(found - d.procedures.begin()) + "]";
    if (found->feedback) checkFeedbackSteps(*found, at + ".feedback"); // D547
    ProcedureRun run;
    run.procedure = *found; run.revision = d.revision;
    const auto& steps = run.procedure.steps;
    for (std::size_t k = 0; k < steps.size(); ++k) {
        ProcedureStepResult step;   // notRun until the loop says otherwise
        step.index = k; step.kind = steps[k].kind;
        run.steps.push_back(std::move(step));
    }
    // The nearest earlier active step of `kind` before `k`; every earlier active step is done (a refusal stops).
    const auto nearest = [&](std::size_t k, ProcedureStepKind kind) -> std::optional<std::size_t> {
        for (std::size_t i = k; i-- > 0;) if (steps[i].active && steps[i].kind == kind) return i;
        return std::nullopt;
    };
    for (std::size_t k = 0; k < steps.size(); ++k) {
        const auto& s = steps[k];
        auto& result = run.steps[k];
        if (!s.active) { result.status = ProcedureStepStatus::skipped; continue; }
        if (observer && observer->step && !observer->step(k)) { run.cancelled = true; break; }
        const auto where = at + ".steps[" + std::to_string(k) + "]";
        std::optional<InputManifest> own;
        if (manifest) own = *manifest;
        InputManifest* m = own ? &*own : nullptr;
        try {
            const auto need = [&](std::size_t from, ProcedureStepKind kind) {
                const auto i = nearest(from, kind);
                if (!i) throw ValidationError({{"PROCEDURE_STEP_INPUT_MISSING", where}});
                return *i;
            };
            switch (s.kind) {
            case ProcedureStepKind::tripGeneration:
                result.generation = runTripGeneration(d, data, s.set, m);
                break;
            case ProcedureStepKind::tripDistribution: {
                const auto& g = steps[need(k, ProcedureStepKind::tripGeneration)];
                result.distribution = runTripDistribution(d, data, g.set, s.set, s.skimMode.value_or(""), s.costMatrix.value_or(""), m);
                break;
            }
            case ProcedureStepKind::modeChoice: {
                const auto t = need(k, ProcedureStepKind::tripDistribution);
                const auto& g = steps[need(t, ProcedureStepKind::tripGeneration)];
                result.modeChoice = runModeChoice(d, data, g.set, steps[t].set, steps[t].skimMode.value_or(""), steps[t].costMatrix.value_or(""),
                                                  s.set, s.periods, m);
                break;
            }
            case ProcedureStepKind::assignment: {
                const auto c = nearest(k, ProcedureStepKind::modeChoice);
                result.macro = assignProcedureStep(d, data, s, c ? &*run.steps[*c].modeChoice : nullptr, m, observer);
                if (result.macro->assignment.cancelled) { result.macro.reset(); run.cancelled = true; }
                break;
            }
            }
        } catch (const ValidationError& e) {
            result.status = ProcedureStepStatus::refused; result.issues = e.issues;
            break;
        } catch (const std::exception& e) {
            result.status = ProcedureStepStatus::refused; result.issues = {{e.what(), where}};
            break;
        }
        if (run.cancelled) break;
        result.status = ProcedureStepStatus::done;
    }
    if (run.procedure.feedback && run.completed()) iterateFeedback(run, d, data, manifest, observer); // D547
    return run;
}
MacroRun assignProcedureStep(const ProjectDocument& d, const std::filesystem::path& data, const ProcedureStep& s,
                             const ModeChoiceRun* modeChoice, InputManifest* m, const ProcedureObserver* observer) {
    // §3.4: the mode choice's matrices replace the generated ones of its periods, in a copy only.
    auto copy = d;
    if (modeChoice) {
        auto& list = copy.zones.odMatrices;
        const auto& periods = modeChoice->periods;
        std::erase_if(list, [&](const auto& x) {
            return x.generatedBy == "mode-choice" &&
                   std::any_of(periods.begin(), periods.end(), [&](const auto& p) { return p.id == x.periodId; });
        });
        for (auto& x : generatedOdMatrices(*modeChoice)) list.push_back(std::move(x));
    }
    const MacroObserver* macroObserver = observer ? &observer->macro : nullptr;
    return s.macroRun ? runNamedMacro(copy, data, *s.macroRun, m, macroObserver) : runMacro(copy, data, *s.period, m, macroObserver);
}
}
