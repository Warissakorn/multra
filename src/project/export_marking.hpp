#pragma once
// S5.5b (STUDY_AREA §10-§11, §8, D332): the program-drawn (`generated`) objects a Micro run over the
// study area rests on, and the marking every export carries when they, or a stale input, are there.
// Pure: the formatters keep their bytes, and callers wrap what they write.
#include "study_area_run.hpp"
#include <optional>

namespace multra {
// F4's objects of one kind each: Connectors, right-of-way objects (waiting lines, conflict areas,
// priority rules, stop controls), signal heads, and the controllers those heads show.
struct GeneratedCount {
    std::size_t connectors{}, rightOfWay{}, heads{}, controllers{};
    std::size_t links{}; // J3b's generated cuts/pockets, including legs crossing the boundary
    std::size_t total() const { return connectors + rightOfWay + heads + controllers + links; }
    bool operator==(const GeneratedCount&) const = default;
};
// Inside the Node, as nodeMembership reads it; priority rules and stop controls by the conflict
// areas and waiting lines they name, controllers by the heads.
GeneratedCount generatedInNode(const ProjectDocument&, const Node&);
// The sum over the study area's Nodes; with no study area every generated object, since Micro then
// runs all of it (§10).
std::size_t generatedInArea(const ProjectDocument&);

struct ExportMarking {
    std::size_t generatedInArea{};
    std::vector<std::string> stale; // "macroSide" / "mesoSide" (the cut's side file), "project" (a result for other bytes)
    bool empty() const { return generatedInArea == 0 && stale.empty(); }
    bool operator==(const ExportMarking&) const = default;
};
// `provenance` is the run's (RunSnapshot::studyArea); `resultStale` a Macro or Meso result made for
// other project bytes.
ExportMarking exportMarking(const ProjectDocument&, const std::optional<StudyAreaProvenance>& provenance, bool resultStale = false);
// `csv` with, right after its first line, the marking line (only when the marking is not empty)
// and the study-area flows line (only with provenance); with neither, `csv` unchanged.
std::string markedCsv(std::string csv, const ExportMarking&, const std::optional<StudyAreaProvenance>& = std::nullopt);
// A comparison: each side's lines, prefixed `base:` and `alternative:`.
std::string markedComparisonCsv(std::string csv, const ExportMarking& base, const ExportMarking& alternative,
                                const std::optional<StudyAreaProvenance>& baseFlows = std::nullopt,
                                const std::optional<StudyAreaProvenance>& alternativeFlows = std::nullopt);
// `"marking"` (only when not empty) and `"studyArea"` (only with provenance), the same rule.
void addMarkingJson(Json&, const ExportMarking&, const std::optional<StudyAreaProvenance>& = std::nullopt);
void addComparisonMarkingJson(Json&, const ExportMarking& base, const ExportMarking& alternative,
                              const std::optional<StudyAreaProvenance>& baseFlows = std::nullopt,
                              const std::optional<StudyAreaProvenance>& alternativeFlows = std::nullopt);
}
