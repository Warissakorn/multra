#pragma once
#include "od_lot_dialogs.hpp"

namespace multra {
// S5.2b (STUDY_AREA §5, D328): the study area's typed gate flows, veh/h from each of `origins` to each
// of `destinations`, for one study period and one composition or mode at a time. Switching either
// shows that key's flows and keeps what was typed; the result is every key's flows (zero cells are
// dropped by setStudyAreaGateFlows). S5.3/S5.4 (D329, D330): a Source choice, typed or cut from Macro or Meso, which
// disables the grid; the typed flows are kept either way. nullopt: cancelled. Local values only.
struct GateFlowEdit { std::string source; std::vector<GateFlow> flows; };
std::optional<GateFlowEdit> editGateFlows(QWidget*, const std::string& source, const std::vector<GateFlow>& current,
                                                   const std::vector<Choice>& origins, const std::vector<Choice>& destinations,
                                                   const std::vector<Choice>& compositions, const std::vector<Choice>& periods,
                                                   const std::vector<Choice>& modes, const DialogText&);
}
