#pragma once
#include "document.hpp"
#include "../model/demand/meso_compile.hpp"
#include "../meso/demand.hpp"
#include <filesystem>

namespace multra {
class InputManifest;
// M11.2 (docs/reference/MESO_NETWORK.md §3, §8, D293): what a Meso run reads from a document. The
// resolved link types (owned or data/link-types/) and the run settings compiled into the graph, with
// INVALID_LINK_TYPE (modes checked against the resolved modes) added to its issues.
MesoCompile mesoNetwork(const ProjectDocument&, const std::filesystem::path& dataDirectory, InputManifest* = nullptr);
// M11.3a (§6, D295): Micro's authored routes as Meso paths, one class per composition
// ("composition:<id>") or lone vehicle type ("type:<id>"), over the whole run. OD matrices expand
// as withOdDemand, static decisions as withRoutingDecisions; inputs on one route and class share a
// path, classes and paths ordered by id. M11.3b (D296): routeless inputs and placed decisions take
// Micro's expandRouteless paths, mapped back to objects; a stub's flow goes to the full routes its
// vehicles change lanes onto, split equally. Refused: MESO_PATH_UNMAPPED at `definition.routes[j]`
// (or the routeless input), or at `odMatrices[k]` for its routes; MODE_PCU_MISSING at the input or
// matrix; the walk's blocking issues at `definition.inputs[i].…`.
struct MesoDemand { std::vector<MesoClass> classes; std::vector<ValidationIssue> issues; bool operator==(const MesoDemand&) const = default; };
MesoDemand mesoDemand(const ProjectDocument&, const std::filesystem::path& dataDirectory, const MesoGraph&, InputManifest* = nullptr);
}
