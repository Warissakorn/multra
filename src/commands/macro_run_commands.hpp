#pragma once
#include "../project/document.hpp"

namespace multra {
// Replaces the list in order; empty ids are allocated with the macro-run prefix. History owns
// validation and atomicity, so accepting the editor's whole list is one undoable command.
void setMacroRunDefinitions(ProjectDocument&, std::vector<MacroRunDefinition>);
}
