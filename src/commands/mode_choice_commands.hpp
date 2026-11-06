#pragma once
// M10.4 E2 (MODE_CHOICE §6-§7, D479): the project's mode-choice sets and period factors, each replaced as one
// History step (validateDocument refuses blank or duplicate ids and unknown periods), and Write OD matrices.
#include "../project/document.hpp"
#include "../project/mode_choice_run.hpp"

namespace multra {
void setModeChoiceSets(ProjectDocument&, std::vector<ModeChoiceSet>);
void setPeriodFactors(ProjectDocument&, std::vector<PeriodFactor>);
// generatedOdMatrices(run) is in src/project/mode_choice_run.hpp since D484 (the procedure run needs it).
// Removes every matrix with `generatedBy == "mode-choice"`, appends `matrices`, and (as the OD dialog does)
// lets a project owning its compositions take `installedModes` as its own. Authored matrices are untouched;
// an id clash with one is refused by validation.
void writeGeneratedOdMatrices(ProjectDocument&, std::vector<OdMatrix> matrices, std::vector<Mode> installedModes);
}
