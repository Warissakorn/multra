#pragma once
// Random streams per input (INPUT_STREAMS §3-§4, D496/D505). Under `perInput` every input and every
// parking lot draws from its own xorshift32 state, keyed by its id, and a routing decision's draw is a
// hash of the vehicle's origin and ordinal, so an edit to one object leaves every other one's draws as
// they were. `shared` (the default) is the single run stream, unchanged to the bit.
#include <cstdint>
#include <string>

namespace multra {
enum class RandomStreams : std::uint8_t { shared, perInput };
namespace detail {
// The kind bytes of §4: one namespace per object kind, so an input and a lot with the same id differ.
inline constexpr char kInputStream = 'I', kLotStream = 'L', kDecisionStream = 'D';
// FNV-1a over the kind byte, then the id's bytes: stable across edits to any other object.
inline std::uint64_t streamKey(char kind, const std::string& id) {
    std::uint64_t h = 0xcbf29ce484222325ULL;
    const auto add = [&](unsigned char c) { h ^= c; h *= 0x100000001b3ULL; };
    add(static_cast<unsigned char>(kind));
    for (const char c : id) add(static_cast<unsigned char>(c));
    return h;
}
// Defined in demand.cpp, beside the run stream's own seeding (D159).
std::uint32_t streamState(std::uint32_t seed, std::uint64_t key);
// One uniform in (0, 1) with no state: hash(seed, origin, ordinal, decision). Order-independent.
double hashedUniform(std::uint32_t seed, std::uint64_t origin, std::uint32_t ordinal, std::uint64_t decision);
// §4 choice 2 (D512): who a vehicle is to W74's trait hash and the red-runner hash. Under perInput its
// origin's key and ordinal, so an added input leaves every other input's traits alone; a vehicle with
// no origin key (every vehicle under shared, and one placed by hand) keeps its id, unchanged to the bit.
std::uint64_t traitIdentity(std::uint64_t vehicleId, std::uint64_t origin, std::uint32_t ordinal);
}
}
