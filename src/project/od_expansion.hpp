#pragma once
// M8.3 (D150, D151; docs/reference/ZONES_AND_OD.md §2-§3): OD matrices become ordinary authored
// routes and vehicle inputs, appended after the authored ones, by fewest-object chains.
#include "document.hpp"
#include "../core/validate.hpp"
#include "../model/network/diagnostics.hpp"
#include <filesystem>
#include <map>
#include <optional>
#include <set>

namespace trafficsim {
class InputManifest;
// The demand classes a matrix may name. nullopt means the list is not known here (an external
// catalog that was not read): every class is accepted and none is reported.
struct OdClasses { std::optional<std::set<std::string>> vehicleTypes, compositions; };
// What the document itself carries, for authoring and the Problems tab (no data directory).
OdClasses ownedOdClasses(const AuthoringDefinition&);
// The project's catalog, read only when a matrix exists (D151); compositions that cannot be used
// (empty, a share not above 0, an unknown type) are left out, so a matrix naming one is unknown.
OdClasses resolvedOdClasses(const AuthoringDefinition&, const std::filesystem::path& dataDirectory,
                            InputManifest* = nullptr);
struct OdCell { std::size_t matrix{}, origin{}, destination{}; };
struct OdExpansion {
    AuthoringDefinition definition;       // authored, then generated routes and inputs; no matrices
    std::vector<ValidationIssue> issues;  // the §2 Run codes, on authored paths, each once
    std::map<std::string, OdCell> routeOrigin, inputOrigin; // generated id -> the first cell using it
};
// One walk: the issues and the definition come from the same pass, so a cell or matrix a row
// names emits nothing. Without matrices the definition is an exact copy.
OdExpansion expandOd(const Network&, const AuthoringDefinition&, const OdClasses&);
AuthoringDefinition withOdDemand(const Network&, const AuthoringDefinition&, const OdClasses&);
std::vector<ValidationIssue> odRunIssues(const Network&, const AuthoringDefinition&, const OdClasses&);
// Run rows for the expansion's issues: a zone connector's row names it, a matrix's row names the
// matrix. Neither is canvas-selectable yet (M8.6).
void addOdRunRows(const Network&, const AuthoringDefinition& authored, const OdExpansion&, std::vector<Diagnostic>&);
// Where a check's path names a route or an input: at its start, or after a conflict zone
// ("conflictZones[z] routes[i]"). `at` is where that part starts.
struct GeneratedRef { bool route{}; std::size_t at{}, index{}; };
std::optional<GeneratedRef> generatedRef(const std::string& path);
// Rows of runtimeDiagnostics(network, checked) that land on a generated route or input are moved
// onto the matrix cell that made it (`odMatrices[m].trips[o][d]`, named by the matrix id), so no
// Problems row names a generated object. `checked` is what runtimeDiagnostics was given.
void remapOdRows(const Network&, const ScenarioDefinition& checked, const AuthoringDefinition& authored,
                 const OdExpansion&, std::vector<Diagnostic>& rows);
}
