#pragma once
#include "document.hpp"
#include "study_area_run.hpp"
#include <filesystem>

namespace multra {
// S4.2 (READINESS §3, D313): what would stop or weaken a run, per level, from the same functions the
// runs call, so a refusal row is listed exactly when that run is refused (row RS2):
// - micro: runDiagnostics (draft and runtime rows are refusals, advisories stay advisories);
// - meso: mesoNetwork's issues, then, when the graph compiles, mesoDemand's (runMeso's stages);
// - macro: for `period` (macroPeriodName; empty or unknown: the first period), macroNetwork's
//   issues, then macroDemand's; no period at all is MACRO_PERIOD_UNKNOWN, as runMacro refuses;
// then each level's advisories, seamChecks and junctionSeamIssues (warnings/advisories). A runtime catalog
// that cannot be read is one EDIT_CATALOG_READ refusal. Never throws; changes nothing.
enum class ReadinessLevel { micro, meso, macro };
enum class ReadinessSeverity { refusal, warning, advisory };
struct ReadinessRow {
    std::string code, path, objectId, selectId; ReadinessSeverity severity{};
    std::vector<std::size_t> counts{}; // S5.5b: MICRO_GENERATED_IN_AREA's by kind (GeneratedCount's order)
    bool operator==(const ReadinessRow&) const = default;
};
// S5.5a (D331): `sources` reach runDiagnostics for a study area's cut. S5.5b (STUDY_AREA §10, D332):
// at Micro, one MICRO_GENERATED_IN_AREA warning at nodes[i] per study-area Node (every Node without
// an area) holding generated objects, naming its Node (the editor selects it), with the counts; the
// run still starts.
std::vector<ReadinessRow> readiness(const ProjectDocument&, const std::filesystem::path& dataDirectory, ReadinessLevel,
                                    const std::string& period = {}, const StudyAreaSources* sources = nullptr);
}
