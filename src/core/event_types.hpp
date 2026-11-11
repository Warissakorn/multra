#pragma once
// The run's event stream (SIMULATION "Events"), apart from the runtime state that emits it. Included
// by types.hpp; a new alternative appends, so every existing variant index stays stable.
#include <cstdint>
#include <string>
#include <variant>

namespace multra {
enum class SignalColor { red, amber, green };
struct SignalEvent {
    double time{}; std::string signalId; SignalColor color{};
    bool operator==(const SignalEvent&) const = default;
};
struct DepartedEvent {
    double time{}; std::uint64_t vehicleId{}; std::string routeId;
    double scheduledTime{}, desiredSpeed{};
    bool operator==(const DepartedEvent&) const = default;
};
struct MovedEvent {
    double time{}; std::uint64_t vehicleId{}; std::string segmentId;
    double position{}, speed{}, acceleration{};
    bool operator==(const MovedEvent&) const = default;
};
struct SegmentEnteredEvent {
    double time{}; std::uint64_t vehicleId{}; std::string segmentId;
    bool operator==(const SegmentEnteredEvent&) const = default;
};
struct SafetyClampEvent {
    double time{}; std::uint64_t vehicleId{};
    bool operator==(const SafetyClampEvent&) const = default;
};
struct ArrivedEvent {
    double time{}; std::uint64_t vehicleId{}; std::string routeId;
    double travelTime{}, departureDelay{}, freeFlowTime{};
    bool operator==(const ArrivedEvent&) const = default;
};
// M3.2.8b: a vehicle moved from one route to another at the start of a tick (contract §2).
struct LaneChangeEvent {
    double time{}; std::uint64_t vehicleId{}; std::string fromRouteId, toRouteId;
    bool operator==(const LaneChangeEvent&) const = default;
};
struct RoutingEvent {
    double time{}; std::uint64_t vehicleId{}; std::string decisionId, fromRouteId, toRouteId;
    bool operator==(const RoutingEvent&) const = default;
};
struct RedRunningEvent { double time{}; std::uint64_t vehicleId{}; std::string headId; bool operator==(const RedRunningEvent&) const = default; }; // B-9 (D238)
// New event alternatives append so every existing index stays stable.
using SimEvent = std::variant<SignalEvent, DepartedEvent, MovedEvent,
                              SegmentEnteredEvent, SafetyClampEvent, ArrivedEvent, LaneChangeEvent, RoutingEvent, RedRunningEvent>;
}
