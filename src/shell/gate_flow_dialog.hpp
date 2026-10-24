#pragma once
#include "od_lot_dialogs.hpp"

namespace multra {
// S5.2b (STUDY_AREA §5, D328): the study area's typed gate flows, veh/h from each of `origins` to each
// of `destinations`, for one study period and one composition or mode at a time. Switching either
// shows that key's flows and keeps what was typed; the result is every key's flows (zero cells are
// dropped by setStudyAreaGateFlows). nullopt: cancelled. Local values only: the caller commits.
std::optional<std::vector<GateFlow>> editGateFlows(QWidget*, const std::vector<GateFlow>& current,
                                                   const std::vector<Choice>& origins, const std::vector<Choice>& destinations,
                                                   const std::vector<Choice>& compositions, const std::vector<Choice>& periods,
                                                   const std::vector<Choice>& modes, const DialogText&);
}
