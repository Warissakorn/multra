#pragma once
#include "od_lot_dialogs.hpp"
#include "../project/document.hpp"

namespace multra {
struct MacroRunChoice { std::string period, definitionId; };
// Local values only. The caller commits an accepted definition list in one History step.
std::optional<std::vector<MacroRunDefinition>> editMacroRunDefinitions(QWidget*, const ProjectDocument&, const DialogText&);
// The legacy project-settings choice stays first. A named choice uses its saved period/settings;
// missing periods remain selectable so the runner can report MACRO_PERIOD_UNKNOWN.
std::optional<MacroRunChoice> chooseMacroRun(QWidget*, const ProjectDocument&, const DialogText&);
}
