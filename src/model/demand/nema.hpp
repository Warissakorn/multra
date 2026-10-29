#pragma once
#include "signal_control.hpp"
#include "../network/approach_layout.hpp"

namespace multra {
// J4, D404: derived per call, never stored. Explicit site clearance and waiting stay separate.
struct NemaPhaseTiming { int number{}; double clearance{}, wait{}; };
struct NemaTiming {
    SignalController controller;
    std::vector<NemaPhaseTiming> phases;
    std::vector<ValidationIssue> issues;
};
std::vector<int> nemaPhases(const std::string& preset);
NemaTiming nemaTiming(const std::string& preset,const JunctionSignal&,double timeStep,
                      const std::string& path="junction.signal");
struct NemaPhaseMap { std::map<std::string,int> through; std::vector<ValidationIssue> issues; };
NemaPhaseMap nemaApproachPhases(const Network&,const ApproachLayout&,const std::vector<std::string>& major);
int nemaThroughPhase(ApproachDirection,bool eastWest);
int nemaFarPhase(int through);
// A read-only view supplies head bindings and the existing derived layout; no second signal model.
struct NemaLayout { const Network& network; const ApproachLayout& approaches; };
std::vector<ValidationIssue> nemaCheck(const SignalController&,const NemaLayout&);
}
