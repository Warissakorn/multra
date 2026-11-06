#pragma once
// Procedure sequence P2 (PROCEDURE_SEQUENCE §4, D485): the project's procedures, replaced as one History step
// (validateDocument refuses a blank or repeated id and a malformed step).
#include "../project/document.hpp"

namespace multra {
void setProcedures(ProjectDocument&, std::vector<Procedure>);
}
