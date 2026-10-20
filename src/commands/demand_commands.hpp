#pragma once
#include "history.hpp"
namespace trafficsim {
AuthoringDefinition& demand(ProjectDocument&);
std::string putRoute(ProjectDocument&, Route); // Empty id allocates; same id replaces.
std::string putInput(ProjectDocument&, VehicleInput);
std::string putProgram(ProjectDocument&, SignalProgram);
void deleteRoute(ProjectDocument&, const std::string&); // Cascades its inputs and decision entries.
// M2.4. Empty id allocates; same id replaces. Validated with the rest of the demand on commit.
std::string putRoutingDecision(ProjectDocument&, RoutingDecision);
void deleteRoutingDecision(ProjectDocument&, const std::string&); // Cascades its inputs.
// B-6 (SPEED_DECISIONS §6, D223): a desired-speed decision; it goes with its Link.
std::string putSpeedDecision(ProjectDocument&, SpeedDecisionSpec);
void deleteSpeedDecision(ProjectDocument&, const std::string&);
std::string putReducedSpeedArea(ProjectDocument&, ReducedSpeedAreaSpec); // B-6 §7 (D225)
void deleteReducedSpeedArea(ProjectDocument&, const std::string&);
void deleteInput(ProjectDocument&, const std::string&);
void deleteProgram(ProjectDocument&, const std::string&); // Reject referenced programs.
// M2.7b. Empty id allocates; same id replaces. Removing a group a head still shows is refused
// on commit (UNKNOWN_SIGNAL_GROUP); deleting a controller a head shows, here.
std::string putSignalController(ProjectDocument&, SignalController);
void deleteSignalController(ProjectDocument&, const std::string&);
void changeRunSettings(ProjectDocument&, double duration, double timeStep);
// M3.2.8a.1 (MERGE_CLAMPS §3, D251): the three project switches; all on writes nothing.
void changeClampRules(ProjectDocument&, ClampRules);
// S2.3 (MACRO_NETWORK §7, D275): nullopt removes the key; a bad value is INVALID_MACRO_SETTING.
void changeMacroSettings(ProjectDocument&, std::optional<MacroSettings>);
// M5.3. Warm-up 0 with no end clears the period, so a file that never used one keeps its schema.
// Sets warm-up and end and keeps the cool-down; the four-argument form sets all three (M5.9).
// The period is removed only when every part is at its default: warm-up 0, no end, no cool-down.
void changeEvaluationPeriod(ProjectDocument&, double warmup, std::optional<double> end);
void changeEvaluationPeriod(ProjectDocument&, double warmup, std::optional<double> end, double cooldown);
std::string putSignalHead(ProjectDocument&, NetworkSignalHead);
void deleteSignalHead(ProjectDocument&, const std::string&);
// Slides a head -- its stop line -- along the lane or path it stands on. The station is
// validated with the network on commit (INVALID_POSITION), never clamped here.
void moveSignalHead(ProjectDocument&, const std::string& id, double position);
}
