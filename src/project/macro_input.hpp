#pragma once
#include "document.hpp"
#include "../model/demand/macro_compile.hpp"
#include <filesystem>

namespace trafficsim {
class InputManifest;
// S2.1 (docs/reference/MACRO_NETWORK.md §3-§6, D273): what a Macro run reads from a document.
// The resolved link types (owned or data/link-types/) and data/vdf/ compiled into the graph, with
// INVALID_LINK_TYPE (modes checked against the resolved modes) and INVALID_VDF added to its issues.
MacroCompile macroNetwork(const ProjectDocument&, const std::filesystem::path& dataDirectory, InputManifest* = nullptr);
// One assignment's demand (§6): a study period that some matrix names, or one own-times matrix.
struct MacroPeriod {
    std::string periodId, matrixId; // exactly one is set
    bool operator==(const MacroPeriod&) const = default;
};
// The named study periods in their order, then each own-times matrix in matrix order.
std::vector<MacroPeriod> macroPeriods(const ProjectDocument&);
// A class: one mode (`mode` set) or one composition (`id` = "composition:<id>", every link), with its
// PCU and its veh/h from each origin centroid to each destination centroid.
struct MacroCell { std::size_t origin{}, destination{}; double vehiclesPerHour{}; bool operator==(const MacroCell&) const = default; };
struct MacroClass {
    std::string id, mode; double pcu{}; std::vector<MacroCell> cells;
    bool operator==(const MacroClass&) const = default;
};
// veh/h = trips x origin share x destination share / period hours, summed over the period's
// matrices of one class. Refused: UNKNOWN_MODE (odMatrices[i].modeId), MODE_PCU_MISSING
// (odMatrices[i]), MACRO_NO_PATH (the cell). Advised: MACRO_INPUT_IGNORED, MACRO_LOT_UNCAPPED.
struct MacroDemand { std::vector<MacroClass> classes; std::vector<ValidationIssue> issues, advisories; };
MacroDemand macroDemand(const ProjectDocument&, const std::filesystem::path& dataDirectory, const MacroPeriod&, const MacroGraph&);
}
