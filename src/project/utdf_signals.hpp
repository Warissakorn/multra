#pragma once
// UTDF U2a (UTDF_EXPORT §7 U2a contract, D439, D443): a written Node's signal controller, checked by
// nemaCheck, and its [Timeplans] and [Phases] records. Internal to utdf_export.cpp.
#include "utdf_legs.hpp"
#include "document.hpp"
#include <array>

namespace multra::utdf {
struct NodeSignal {
    enum class Kind { unsignalised, valid, invalid } kind{Kind::unsignalised};
    const SignalController* controller{};
    std::map<std::size_t, int> groups;  // membership turn -> signal group number (valid only)
    std::vector<ValidationIssue> issues; // invalid only
};
NodeSignal nodeSignal(const ProjectDocument&, const WrittenNode&);
using Record = std::pair<std::string, std::string>;
using PhaseRecord = std::pair<std::string, std::array<std::string, 8>>; // D1..D8
struct TimingRecords { std::vector<Record> timeplans; std::vector<PhaseRecord> phases; std::vector<ValidationIssue> issues; };
// Every Synchro 10.1 record, in its order; issues (a negative all-red) mean nothing may be written.
TimingRecords timingRecords(const SignalController&, int nodeNumber);
}
