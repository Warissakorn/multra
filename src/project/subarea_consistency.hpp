#pragma once
// M12 R1 (docs/reference/SUBAREA_CONSISTENCY.md, D553): Meso on a study area's run-time cut, and the
// cross-level consistency report. Not validated (rule 4): the report says how far the levels are apart,
// never which one is right, and applies no threshold.
#include "meso_run.hpp"
#include "study_area_run.hpp"
#include "export_marking.hpp"
#include "run.hpp"
#include "../runner/batch.hpp"
#include <array>

namespace multra {
// §3: Meso over studyAreaDocument's copy, the cut Micro runs on, with every kept Link watched. Refused:
// MESO_AREA_SOURCE_SELF at studyArea.flowSource for a "meso" source, then the cut's own refusals; a
// document without a study area is CONSISTENCY_NO_STUDY_AREA at studyArea. The copy's advisories come
// first in the run's. runMeso itself is unchanged.
MesoRun runMesoArea(const ProjectDocument&, const std::filesystem::path& dataDirectory, const StudyAreaSources* = nullptr,
                    StudyAreaProvenance* = nullptr);
// §5: the coarser value a, the finer b. Empty Δ, % and GEH when a side is empty; % empty when a = 0,
// GEH empty when a + b = 0.
struct ConsistencyComparison {
    std::optional<double> delta, percent, geh;
    bool operator==(const ConsistencyComparison&) const = default;
};
ConsistencyComparison compareVolumes(std::optional<double> coarse, std::optional<double> fine);
enum class ConsistencyKind { entryGate, exitGate, link };
struct ConsistencyRow {
    ConsistencyKind kind{};
    std::string id, name;          // the gate's name or the Link's id; the Link's name
    std::optional<double> macro, meso; // veh/h
    Estimate micro;                // veh/h per seed
    std::array<ConsistencyComparison, 3> comparisons; // (Macro, Meso), (Macro, Micro), (Meso, Micro)
};
struct ConsistencyPeriod { std::string id; double start{}, end{}; std::vector<ConsistencyRow> rows; };
struct ConsistencyReport {
    std::string project, projectSha256, flowSource;
    StudyAreaProvenance provenance; // the Micro cut's, as compileDocument records it
    ExportMarking marking;
    std::vector<std::uint32_t> seeds;
    MesoSettings mesoSettings;
    double clockStart{}, duration{};
    std::vector<ConsistencyPeriod> periods;
    std::vector<ValidationIssue> advisories;
    bool cancelled{}; // R2 (§13): `cancelled` fired; no periods
};
// §4: per study period wholly inside the run, every entry gate, exit gate and kept Link. `project` is
// the file name written into the provenance. Refused as runMesoArea, and by the Micro compile. R2 (§13):
// `cancelled`, when given, is polled before Meso and before each seed; true returns a cancelled report.
ConsistencyReport consistencyReport(const ProjectDocument&, const std::filesystem::path& dataDirectory,
                                    const StudyAreaSources&, const std::vector<std::uint32_t>& seeds, const std::string& project,
                                    const std::function<bool()>& cancelled = {});
// Micro's volumes (veh/h) for one seed of a compiled run, one map per window [from, to) of run time:
// Link id -> vehicles entering it, exitGateName(link) -> arrivals on a route ending on it. Exposed for SC5.
std::vector<std::map<std::string, double>> microVolumes(const RunSnapshot&, std::uint32_t seed,
                                                        const std::vector<std::pair<double, double>>& windows);
Json consistencyJson(const ConsistencyReport&);
std::string consistencyCsv(const ConsistencyReport&);
}
