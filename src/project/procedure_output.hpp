#pragma once
// Procedure sequence P1 (PROCEDURE_SEQUENCE §4, D484): the run's JSON. Each step's result is its unit's own
// writer output (trip generation, distribution, mode choice, Macro), or null.
#include "procedure_run.hpp"

namespace multra {
Json procedureJson(const ProcedureRun&);
}
