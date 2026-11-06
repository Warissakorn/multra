#pragma once
// M10.4 E2 (MODE_CHOICE §5-§7, D479): the project's period factors as an editable table (purpose, study period,
// P->A share, A->P share) with Add and Remove. Local until accepted; the caller commits the list in one History
// step, where validateDocument refuses an unknown period or a repeated pair. A cell that is not a share in
// [0, 1], or a row whose shares exceed 1, keeps the dialog open with PERIOD_FACTOR_INVALID at the row.
#include "od_lot_dialogs.hpp"
#include "../project/document.hpp"
#include <optional>
#include <vector>

namespace multra {
std::optional<std::vector<PeriodFactor>> editPeriodFactors(QWidget*, const ProjectDocument&, const DialogText&);
}
