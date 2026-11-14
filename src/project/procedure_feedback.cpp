// PROCEDURE_FEEDBACK §2-§3 (D547): congested costs back into a procedure's demand steps. After the plain pass
// (iteration 1), each iteration skims at the last assignment's link times, runs the mode choice (with its
// distribution) again for the auxiliary matrices e^n, averages them with T^(n-1) by the book's constant weight
// or MSA (Ortuzar and Willumsen 2011, section 11.3.3), assigns the average, and stops on total misplaced flow.
#include "procedure_run.hpp"
#include "input_manifest.hpp"
#include <cmath>
#include <stdexcept>

namespace multra {
namespace {
std::vector<std::size_t> activeOf(const Procedure& p, ProcedureStepKind kind) {
    std::vector<std::size_t> out;
    for (std::size_t k = 0; k < p.steps.size(); ++k) if (p.steps[k].active && p.steps[k].kind == kind) out.push_back(k);
    return out;
}
std::size_t lastActive(const Procedure& p) {
    std::size_t last = p.steps.size();
    for (std::size_t k = 0; k < p.steps.size(); ++k) if (p.steps[k].active) last = k;
    return last;
}
void blend(DistributionMatrix& t, const DistributionMatrix& e, double w) {
    if (t.size() != e.size()) throw std::logic_error("feedback: matrices of different shapes");
    for (std::size_t i = 0; i < t.size(); ++i) {
        if (t[i].size() != e[i].size()) throw std::logic_error("feedback: matrices of different shapes");
        for (std::size_t j = 0; j < t[i].size(); ++j) t[i][j] = w * t[i][j] + (1 - w) * e[i][j];
    }
}
// T^n = w T^(n-1) + (1 - w) e^n, built on e's structure; persons, each alternative's vehicles and each mode's vehicles
// are all linear in the persons, so averaging each equals averaging the persons and dividing by occupancy.
ModeChoiceRun average(const ModeChoiceRun& previous, const ModeChoiceRun& e, double w) {
    auto t = e;
    if (t.periods.size() != previous.periods.size()) throw std::logic_error("feedback: periods differ between iterations");
    for (std::size_t p = 0; p < t.periods.size(); ++p) {
        auto& now = t.periods[p];
        const auto& before = previous.periods[p];
        if (now.alternatives.size() != before.alternatives.size() || now.modes.size() != before.modes.size())
            throw std::logic_error("feedback: alternatives differ between iterations");
        for (std::size_t a = 0; a < now.alternatives.size(); ++a) {
            auto persons = before.alternatives[a].persons;
            blend(persons, now.alternatives[a].persons, w);
            now.alternatives[a].persons = std::move(persons);
            if (now.alternatives[a].vehicles && before.alternatives[a].vehicles) {
                auto vehicles = *before.alternatives[a].vehicles;
                blend(vehicles, *now.alternatives[a].vehicles, w);
                now.alternatives[a].vehicles = std::move(vehicles);
            }
        }
        for (std::size_t m = 0; m < now.modes.size(); ++m) {
            auto vehicles = before.modes[m].vehicles;
            blend(vehicles, now.modes[m].vehicles, w);
            now.modes[m].vehicles = std::move(vehicles);
        }
    }
    return t;
}
// TMF (Eq. 11.28) and RSE (Eq. 11.29) between T^(n-1) and e^n over every period, alternative and cell, in persons.
std::pair<double, double> misplaced(const ModeChoiceRun& previous, const ModeChoiceRun& e) {
    double tmf = 0, squares = 0;
    for (std::size_t p = 0; p < e.periods.size(); ++p)
        for (std::size_t a = 0; a < e.periods[p].alternatives.size(); ++a) {
            const auto& x = previous.periods[p].alternatives[a].persons;
            const auto& y = e.periods[p].alternatives[a].persons;
            for (std::size_t i = 0; i < y.size(); ++i)
                for (std::size_t j = 0; j < y[i].size(); ++j) {
                    const double d = x[i][j] - y[i][j];
                    tmf += std::abs(d); squares += d * d;
                }
        }
    return {tmf, std::sqrt(squares)};
}
FeedbackIteration iterationOf(int n, const MacroRun& macro) {
    FeedbackIteration it; it.n = n;
    it.assignmentIterations = macro.assignment.iterations; it.relativeGap = macro.assignment.gap;
    return it;
}
}
void checkFeedbackSteps(const Procedure& p, const std::string& at) {
    const auto t = activeOf(p, ProcedureStepKind::tripDistribution), c = activeOf(p, ProcedureStepKind::modeChoice),
               a = activeOf(p, ProcedureStepKind::assignment);
    if (t.size() != 1 || c.size() != 1 || a.size() != 1 || !(t[0] < c[0] && c[0] < a[0]) || a[0] != lastActive(p))
        throw ValidationError({{"FEEDBACK_STEPS_INVALID", at}});
}
void iterateFeedback(ProcedureRun& run, const ProjectDocument& d, const std::filesystem::path& data, const InputManifest* manifest,
                     const ProcedureObserver* observer) {
    const auto& p = run.procedure;
    const auto& settings = *p.feedback;
    const auto t = activeOf(p, ProcedureStepKind::tripDistribution)[0], c = activeOf(p, ProcedureStepKind::modeChoice)[0],
               a = activeOf(p, ProcedureStepKind::assignment)[0];
    std::size_t g = t;
    while (g-- > 0) if (p.steps[g].active && p.steps[g].kind == ProcedureStepKind::tripGeneration) break;
    const auto& distribution = p.steps[t];
    FeedbackRun feedback; feedback.settings = settings;
    feedback.iterations.push_back(iterationOf(1, *run.steps[a].macro));
    for (int n = 2; n <= settings.maxIterations; ++n) {
        if (observer && observer->step && !observer->step(c)) { run.cancelled = true; break; }
        std::optional<InputManifest> own;
        if (manifest) own = *manifest;
        InputManifest* m = own ? &*own : nullptr;
        const double w = settings.method == FeedbackMethod::msa ? static_cast<double>(n - 1) / n : *settings.weight;
        std::size_t at = c;
        try {
            const auto times = run.steps[a].macro->assignment.time;
            const auto e = runModeChoice(d, data, p.steps[g].set, distribution.set, distribution.skimMode.value_or(""),
                                         distribution.costMatrix.value_or(""), p.steps[c].set, p.steps[c].periods, m, {}, &times);
            const auto [tmf, rse] = misplaced(*run.steps[c].modeChoice, e);
            auto averaged = average(*run.steps[c].modeChoice, e, w);
            at = a;
            auto macro = assignProcedureStep(d, data, p.steps[a], &averaged, m, observer);
            if (macro.assignment.cancelled) { run.cancelled = true; break; }
            run.steps[t].distribution = e.distribution;
            run.steps[c].modeChoice = std::move(averaged);
            run.steps[a].macro = std::move(macro);
            auto it = iterationOf(n, *run.steps[a].macro);
            it.weight = w; it.tmf = tmf; it.rse = rse;
            feedback.iterations.push_back(it);
            if (tmf <= settings.tolerance) { feedback.converged = true; break; }
        } catch (const ValidationError& e) {
            run.steps[at].status = ProcedureStepStatus::refused; run.steps[at].issues = e.issues;
            break;
        } catch (const std::exception& e) {
            run.steps[at].status = ProcedureStepStatus::refused;
            run.steps[at].issues = {{e.what(), "procedures.feedback"}};
            break;
        }
    }
    run.feedback = std::move(feedback);
}
}
