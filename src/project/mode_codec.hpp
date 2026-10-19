#pragma once
#include "document.hpp"
#include <filesystem>
#include <optional>

namespace trafficsim {
class InputManifest;
// S1.2 (STUDY_PERIODS §5-§7, D269, D270): owned `definition.modes`, a matrix's `modeId` and a vehicle
// type's `pcu`. Each is written only when used, which is what makes a file schema 49; below 49 each is
// EDIT_UNSUPPORTED_FIELD at its path.
bool usesModes(const ProjectDocument&);
void rejectModesBefore49(const Json& file);
// Exact keys {id, name, compositionId}, for a catalog file and an owned entry alike.
Mode parseMode(const Json&);
Json modeJson(const Mode&);
// Exact keys {value, source}; anything else is INVALID_PCU at vehicleType.pcu. Range and citation are
// the validator's (INVALID_PCU at the type's path).
std::optional<Pcu> parsePcu(const Json& vehicleType);
void addPcuJson(const VehicleType&, Json& vehicleType);
// §7: Σ shareᵢ·pcuᵢ / Σ shareᵢ over the composition's types; absent when the composition is unknown,
// has time intervals, or any of its types has no pcu.
// The modes a project resolves (D269): its own, or the installed `data/modes/` files in name order
// (none when the directory is missing; a malformed file is EDIT_CATALOG_READ). `path` is where a
// mode naming an unknown composition is reported (MODE_COMPOSITION_UNKNOWN).
struct ResolvedMode { Mode mode; std::string path; };
std::vector<ResolvedMode> resolveModes(const AuthoringDefinition&, const std::filesystem::path& dataDirectory, InputManifest* = nullptr);
std::vector<Mode> modesOf(const std::vector<ResolvedMode>&);
std::optional<double> modePcu(const Mode&, const std::vector<Composition>&, const std::vector<VehicleType>&);
}
