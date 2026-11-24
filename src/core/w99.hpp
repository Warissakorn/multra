#pragma once
#include "following.hpp"
#include "w99_types.hpp"
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace multra {
// Wiedemann-99-structured car-following (M3.3.3b, D637). The contract, including every equation,
// equality side and the deviations from its sources, is docs/reference/W99.md. This slice is the
// pure function only: no codec, trait draw or composition into the tick yet. Not validated.

// §3's values for one follower and obstacle, exposed so callers and tests read the same numbers
// the classification compares against. Without an obstacle only `vSlow` is meaningful (= speed).
struct W99Thresholds { double vSlow, sdxc, sdxo, sdxv, sdv, sdvc, sdvo; };
W99Thresholds w99Thresholds(double speed, double zW, const W99Parameters& parameters, const Leader& obstacle);

// §4's rows, in classification order; `free` covers both row 0 (no obstacle) and row 4.
enum class W99Regime : std::uint8_t { free, tooClose, closing, following };
struct W99Result {
    double acceleration{};
    W99Regime regime{};
    FollowingMode mode{};
};
// `previousAcceleration` is aPrev (§2): the vehicle's published acceleration from the previous
// tick, 0 at insertion. `timeStep` is the run's dt, read only by §4's standstill hold (D650).
// The result is bounded by the type's limits and D105 (§6), as W74's is; integration, caps and
// Stop rest remain the caller's.
W99Result w99Acceleration(double speed, double desiredSpeed, double previousAcceleration, double timeStep,
    double driverFactor, double zW, const VehicleType& type, const W99Parameters& parameters,
    std::optional<Leader> obstacle = {});

// §5's driver trait zW: W74 §5's hash at index 15, one past W74's 0–14, so a vehicle carrying both
// models' traits has independent draws; integer arithmetic only, and the run's stream is never touched.
inline constexpr std::uint64_t kW99TraitIndex = 15;
double w99Trait(std::uint32_t seed, std::uint64_t vehicleId);
// §5a (D647): the driver's CC1 share, index 16 (one past zW), drawn only when a curved behaviour exists.
inline constexpr std::uint64_t kW99Cc1Index = 16;
double w99Cc1Share(std::uint32_t seed, std::uint64_t vehicleId);
// The curve's inverse at share u (SPEED_DISTRIBUTION §3's rule, shared).
double w99Cc1At(const std::vector<W99Cc1Point>& curve, double u);
// The parameters one driver runs with: cc1 is the curve's value at the driver's share when the
// behaviour has a curve (a missing share there is a logic error), else the behaviour's own.
W99Parameters w99DriverParameters(const W99Parameters& p, std::optional<double> cc1Share);

// The ten keys in §5 order: the one list the codec, the validator and the serializer will read,
// so a key name exists once (hard rule 3).
struct W99Key { const char* name; double W99Parameters::* member; };
const std::vector<W99Key>& w99ParameterKeys();
// §5 ranges and finiteness: INVALID_BEHAVIOUR_PARAMETER at `<path>.<key>`.
std::vector<ValidationIssue> w99ParameterIssues(const W99Parameters& parameters, const std::string& path);
}
