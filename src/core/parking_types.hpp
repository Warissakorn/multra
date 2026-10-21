#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace multra {
// M8.4a (D156, docs/reference/ZONES_AND_PARKING.md §4): a parking lot as the engine runs it. The
// project compiles an authored ParkingLot into this (M8.4b); ids, not slots, because
// createSimulation re-sorts routes, inputs and types and the index resolves them once.
// A cumulative dwell curve, inverted piecewise-linearly by one uniform draw: seconds ascending,
// cumulative ascending and ending at 1. A draw at or below the first share takes its seconds.
struct DwellPoint {
    double seconds{}, cumulative{};
    bool operator==(const DwellPoint&) const = default;
};
// One way out: a departing vehicle takes `routeId`, waiting in the queue of `inputId` -- an input
// on the exit's source segment, usually of 0 veh/h -- chosen by relative `weight`.
struct LotExit {
    std::string routeId, inputId; double weight{};
    bool operator==(const LotExit&) const = default;
};
struct LotType {
    std::string vehicleTypeId; double share{};
    bool operator==(const LotType&) const = default;
};
struct ParkingLotSpec {
    std::string id;
    std::uint32_t capacity{}, initialOccupancy{};
    double gateServiceTime{};               // seconds standing at the line before admission
    std::vector<DwellPoint> dwell;
    std::vector<std::string> gateRouteIds;  // a vehicle on one of these ends its trip at the gate
    std::vector<LotExit> exits;
    std::vector<LotType> initialTypes;      // the types of the vehicles parked at t = 0
    bool operator==(const ParkingLotSpec&) const = default;
};
}
