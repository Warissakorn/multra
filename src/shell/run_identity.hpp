#pragma once
#include <QString>
#include <cstdint>
namespace multra {
// Metadata of the existing GUI single run, separate from deterministic engine state.
struct RunIdentity {
    QString id, lastId, lastEndReason{"notStarted"};
    std::uint64_t generation{}, revision{};
};
}
