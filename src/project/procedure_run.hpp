#pragma once
// Procedure sequence P1 (PROCEDURE_SEQUENCE §3, D484): one run of a named procedure. Each active step runs the
// existing unit with its inputs' parameters (the units are deterministic, so a step's result equals that unit
// run alone); an assignment runs on an in-memory copy of the document whose generated matrices are the nearest
// earlier mode choice's. A refused step stops every later step; the document is never changed.
#include "document.hpp"
#include "macro_run.hpp"
#include "mode_choice_run.hpp"
#include "procedure_codec.hpp"
#include <filesystem>
#include <functional>
#include <optional>

namespace multra {
class InputManifest;
enum class ProcedureStepStatus { done, skipped, refused, notRun };
const char* procedureStepStatusName(ProcedureStepStatus);
struct ProcedureStepResult {
    std::size_t index{};
    ProcedureStepKind kind{};
    ProcedureStepStatus status{ProcedureStepStatus::notRun};
    std::vector<ValidationIssue> issues;        // a refused step's; empty otherwise
    std::optional<TripGenerationRun> generation;
    std::optional<TripDistributionRun> distribution;
    std::optional<ModeChoiceRun> modeChoice;
    std::optional<MacroRun> macro;
};
struct ProcedureRun {
    Procedure procedure;
    std::uint64_t revision{};
    std::vector<ProcedureStepResult> steps;     // one per step, in order
    bool cancelled{};
    bool completed() const;                     // no step refused or left not run
};
struct ProcedureObserver {
    std::function<bool(std::size_t)> step;      // before each active step; false cancels it and the rest
    MacroObserver macro;                        // passed to each assignment; its cancel stops the rest
};
// PROCEDURE_UNKNOWN at `procedures` for an unknown id (thrown). Every later refusal is recorded on its step:
// PROCEDURE_STEP_INPUT_MISSING at `procedures[i].steps[k]` when no earlier active step of the needed kind exists,
// otherwise the unit's own codes and paths. `manifest`, when given, seeds each step's own manifest (the project
// file already recorded), so a step's inputManifest equals that unit's CLI run.
ProcedureRun runProcedure(const ProjectDocument&, const std::filesystem::path& dataDirectory, const std::string& id,
                          const InputManifest* manifest = nullptr, const ProcedureObserver* = nullptr);
}
