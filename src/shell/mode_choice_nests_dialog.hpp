#pragma once
// M10.4 N1 (NESTED_LOGIT §5, D481): the nests of one mode-choice set, edited in place from the Mode choice sets
// dialog. A table of purpose, nest id, phi and members (comma-separated alternative ids), with Add and
// Remove. OK rebuilds every purpose's `nests` and validates the set with the codec; a refusal shows its code
// and path and keeps the dialog open. Local: the sets dialog's Confirm commits the list in one History step.
#include "od_lot_dialogs.hpp"
#include "../project/json.hpp"
#include <optional>

namespace multra {
// `set` and the result are a set's JSON form (modeChoiceSetJson); nullopt on Cancel.
std::optional<Json> editModeChoiceNests(QWidget*, const Json& set, const DialogText&);
}
