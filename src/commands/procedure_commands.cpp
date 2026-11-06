#include "procedure_commands.hpp"

namespace multra {
void setProcedures(ProjectDocument& d, std::vector<Procedure> procedures) { d.procedures = std::move(procedures); }
}
