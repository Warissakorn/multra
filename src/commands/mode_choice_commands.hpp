#pragma once
// M10.4 E2 (MODE_CHOICE §6-§7, D479): the project's mode-choice sets and period factors, each replaced as one
// History step (validateDocument refuses blank or duplicate ids and unknown periods), and Write OD matrices.
#include "../project/document.hpp"
#include "../project/mode_choice_run.hpp"

namespace multra {
void setModeChoiceSets(ProjectDocument&, std::vector<ModeChoiceSet>);
void setPeriodFactors(ProjectDocument&, std::vector<PeriodFactor>);
// One matrix per period and mode of the run: id `mc-<periodId>-<modeId>`, the period, the mode,
// `generatedBy: "mode-choice"`, the run's zone order and its vehicle cells (vehicle trips in the period).
std::vector<OdMatrix> generatedOdMatrices(const ModeChoiceRun&);
// Removes every matrix with `generatedBy == "mode-choice"`, appends `matrices`, and (as the OD dialog does)
// lets a project owning its compositions take `installedModes` as its own. Authored matrices are untouched;
// an id clash with one is refused by validation.
void writeGeneratedOdMatrices(ProjectDocument&, std::vector<OdMatrix> matrices, std::vector<Mode> installedModes);
}
