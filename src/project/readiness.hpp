#pragma once
#include "document.hpp"
#include <filesystem>

namespace multra {
// S4.2 (READINESS §3, D313): what would stop or weaken a run, per level, from the same functions the
// runs call, so a refusal row is listed exactly when that run is refused (row RS2):
// - micro: runDiagnostics (draft and runtime rows are refusals, advisories stay advisories);
// - meso: mesoNetwork's issues, then, when the graph compiles, mesoDemand's (runMeso's stages);
// - macro: for `period` (macroPeriodName; empty or unknown: the first period), macroNetwork's
//   issues, then macroDemand's; no period at all is MACRO_PERIOD_UNKNOWN, as runMacro refuses;
// then each level's advisories, then seamChecks for that level (warnings and advisories). A catalog
// that cannot be read is one EDIT_CATALOG_READ refusal. Never throws; changes nothing.
enum class ReadinessLevel { micro, meso, macro };
enum class ReadinessSeverity { refusal, warning, advisory };
struct ReadinessRow {
    std::string code, path, objectId, selectId; ReadinessSeverity severity{};
    bool operator==(const ReadinessRow&) const = default;
};
std::vector<ReadinessRow> readiness(const ProjectDocument&, const std::filesystem::path& dataDirectory, ReadinessLevel,
                                    const std::string& period = {});
}
