#pragma once
// Procedure sequence P1 (PROCEDURE_SEQUENCE §2, D483-D484): the project's named procedures, root `procedures` at
// schema 73. Each is an ordered list of steps of four kinds; a step takes the nearest earlier active step of
// the kind it needs as its input (checked by a run, as are every set, matrix, mode, period and run id).
#include "json.hpp"
#include "../core/validate.hpp"
#include <optional>
#include <string>
#include <vector>

namespace multra {
enum class ProcedureStepKind { tripGeneration, tripDistribution, modeChoice, assignment };
struct ProcedureStep {
    ProcedureStepKind kind{};
    bool active{true};                        // `active: false` is written only when false
    std::string set;                          // rateSet, deterrenceSet or modeChoiceSet; empty for an assignment
    std::optional<std::string> skimMode, costMatrix;   // tripDistribution only
    std::vector<std::string> periods;         // modeChoice only; empty: every period some factor names
    std::optional<std::string> period, macroRun;       // assignment: exactly one
    bool operator==(const ProcedureStep&) const = default;
};
// PROCEDURE_FEEDBACK §2 (D547), schema 75: congested costs back into the demand steps, averaged by a constant
// weight (the book's w, on the previous matrix) or MSA, stopped by total misplaced flow or the iteration limit.
enum class FeedbackMethod { constantWeight, msa };
struct ProcedureFeedback {
    FeedbackMethod method{FeedbackMethod::constantWeight};
    std::optional<double> weight;             // constantWeight only, in [0, 1)
    int maxIterations{};                      // [2, 1000]
    double tolerance{};                       // TMF, persons per hour, >= 0
    bool operator==(const ProcedureFeedback&) const = default;
};
struct Procedure {
    std::string id, name;
    std::vector<ProcedureStep> steps;
    std::optional<ProcedureFeedback> feedback;
    bool operator==(const Procedure&) const = default;
};
const char* procedureStepKindName(ProcedureStepKind);
const char* feedbackMethodName(FeedbackMethod);
// Every shape fault at once: INVALID_PROCEDURE at the path (unknown kind or key, a missing or blank parameter,
// both or neither of period and macroRun, empty steps or periods), INVALID_ID / DUPLICATE_ID for procedure ids.
// A present list is a non-empty array (INVALID_PROCEDURE at `procedures`); absent or empty is not written.
std::vector<Procedure> parseProcedures(const Json& file);
void addProceduresJson(const std::vector<Procedure>&, Json& file);
std::vector<ValidationIssue> procedureIssues(const std::vector<Procedure>&);
}
