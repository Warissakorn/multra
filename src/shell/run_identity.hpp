#pragma once
#include <QString>
#include <QElapsedTimer>
#include <cstdint>
namespace multra {
// Metadata of the existing GUI single run, separate from deterministic engine state.
struct RunUntilState {
    bool active{};
    QString runId, outcome{"none"};
    std::uint64_t targetTick{};
    int timeoutMs{};
    QElapsedTimer deadline;
};
struct RunIdentity {
    RunUntilState until;
    QString id, lastId, lastEndReason{"notStarted"};
    std::uint64_t generation{}, revision{};
};
}
