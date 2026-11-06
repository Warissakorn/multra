#pragma once
// Procedure sequence P2 (PROCEDURE_SEQUENCE §4, D485): *Procedures…*. A list of the project's procedures (Add,
// Remove, id and name) and, for the selected one, its steps: active, kind, input (rate set, deterrence set,
// mode-choice set, or the assignment's period / `run:<id>` for a Macro run definition), skim mode or periods
// (comma-separated), cost matrix; Add, Remove, Up, Down. OK checks the list (procedureIssues) and shows a
// refusal's code and path in place. Local until accepted; the caller commits it in one History step.
#include "od_lot_dialogs.hpp"
#include "../project/document.hpp"
#include <optional>

namespace multra {
struct ProcedureChoices {
    QStringList rateSets, deterrenceSets, modeChoiceSets;
    QStringList periods;   // study period ids and `matrix:<id>`
    QStringList macroRuns; // definition ids, offered as `run:<id>`
};
std::optional<std::vector<Procedure>> editProcedures(QWidget*, const ProjectDocument&, const ProcedureChoices&, const DialogText&);
}
