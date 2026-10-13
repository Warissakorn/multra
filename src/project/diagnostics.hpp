#pragma once
#include "document.hpp"
#include "../model/network/diagnostics.hpp"

namespace trafficsim {
// Draft issues first, then non-blocking runtime issues. Never throws: parse and compile
// failures come back as rows, so a diagnostics pass cannot take the editor down.
// A definition holding only run settings (a new project, M5.3): no route, input, decision or
// signal timing yet. Diagnostics treat it as a drawing, exactly like no definition (D132).
// A matrix with trips counts as demand (M8.2), so a project whose only demand is one is not this.
bool onlyRunSettings(const AuthoringDefinition&);
// M8.2 only: an OD_NOT_EXPANDED row per matrix with trips, named by the matrix id, not selectable.
void addOdNotExpandedRows(const AuthoringDefinition&, std::vector<Diagnostic>&);
std::vector<Diagnostic> documentDiagnostics(const ProjectDocument& document);
}
