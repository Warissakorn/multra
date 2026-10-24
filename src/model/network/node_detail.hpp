#pragma once
// S4.5a (READINESS §13, D323): F4's plan for a coarse Node's Micro detail. Pure, and never throws.
#include "network.hpp"
#include <map>

namespace multra {
// A Node's arm: the Road a leg Link belongs to, else the Link itself.
std::string legArm(const Link&);
// F4-1a (§13.2): where each leg Link ends (an in-leg) or starts (an out-leg). An in-leg ends where its
// lane surface first meets another arm's leg surface; an out-leg starts where its surface last meets
// one; a leg meeting none keeps its end. `full` gives a leg's uncut geometry (its template's, so a
// re-apply cuts the same way); a leg it does not name is measured as it stands. `geometry` is the
// leg's geometry after the cut. `reason` refuses the whole Node:
// - FILL_NODE_CUT_OUTSIDE: a cut at or outside the polygon's boundary, so the leg would leave it;
// - FILL_NODE_CUT_UNMEASURED: a pair of surfaces surfaceOverlaps cannot measure (`unsupported`).
struct LegCut {
    std::string linkId; bool in{}; double station{}; std::vector<Point> geometry;
    bool operator==(const LegCut&) const = default;
};
struct NodeCuts { std::string reason; std::vector<LegCut> cuts; };
NodeCuts nodeLegCuts(const Network&, const Node&, const std::map<std::string, std::vector<Point>>& full);
// F4-2a (§13.3): one Connector per allowed turn (an in-leg to an out-leg of another arm, less the
// Node's bannedTurns), read on the network as cut. Lanes count from index 0, the kerb. The turns of
// one in-leg are ordered kerb turn first by their sweep (`drivingSide`); with k turns over n lanes,
// turn i takes lanes floor(i*n/k) to ceil((i+1)*n/k) - 1. The out end takes as many lanes, capped at
// its count, from the kerb, except the far turn of two or more, which takes them from the far side.
// `sweep` is the turn's angle towards the kerb, in radians: positive for a kerb turn, negative for a
// far turn, near zero for a through movement (S4.5b reads only its sign).
struct TurnConnector {
    std::string inLink, outLink; int fromLane{}, fromCount{}, toLane{}, toCount{}; double sweep{};
    bool operator==(const TurnConnector&) const = default;
};
std::vector<TurnConnector> nodeTurnConnectors(const Network&, const Node&);
// F4-3a (§13.4): HCM 7th ed., Ch. 20 §2 "Relative Priority of Various Movements" (PDF p. 1590), read
// for any driving side with kerb for the HCM's right and far for its left. Rank 1: major through
// and major kerb turn; 2: major far turn and minor kerb turn; 3: minor through, and the minor far
// turn at a three-arm Node; 4: the minor far turn at a four-arm Node. Through is a turn to the
// other arm of the same class (major to major, minor to minor); otherwise `sweep`'s sign decides.
int movementRank(bool inMajor, bool outMajor, double sweep, std::size_t arms);
// F4-5a (§13.5): the coarse stages laid out in their stored order, each followed by an intergreen
// of (cycle - the greens' sum) / stages, as equal as the run's `step` allows: each is that share
// rounded down to the step, and the last takes what remains. `intergreen` is the shortest and
// `starts` each stage's green start. `offGrid` names a cycle or green the step cannot time.
struct SignalLayout { double intergreen{}; std::vector<double> starts; bool offGrid{}; };
SignalLayout signalLayout(const CoarseSignal&, double step);
// A turn's green over the stages serving it, from the first stage's start to the last one's end;
// `end` < `start` when it wraps past the cycle's end. `reason` refuses it: FILL_NODE_TURN_UNSERVED
// when no stage serves the turn, FILL_NODE_STAGES_SPLIT when its stages are not consecutive (the
// last stage running on into the first counts as consecutive).
struct TurnGreen { std::string reason; double start{}, end{}; std::vector<std::size_t> stages; };
TurnGreen turnGreen(const CoarseSignal&, const SignalLayout&, const std::pair<std::string, std::string>& turn);
}
