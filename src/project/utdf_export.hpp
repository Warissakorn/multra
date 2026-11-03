#pragma once
// UTDF U1 (UTDF_EXPORT §7 U1 contract, D437, D440, D442): the UTDF 8 combined CSV's geometry
// sections, [Network], [Nodes], [Links] and [Lanes], and since U2a (D443) volumes, [Timeplans] and
// [Phases], for Synchro 12 and Vistro 2026. Pure: reads the document and never changes it.
#include <array>
#include <functional>
#include <string>
#include <vector>

namespace multra {
struct ProjectDocument;
// Geometric keeps compass legs and the movement names L/T/R as drawn; mirrored reflects x -> -x,
// so E and W swap in every column and L and R swap in movement names (D437).
enum class UtdfMode { geometric, mirrored };
// One report line: a code, the Node id it concerns (empty for the file) and what was done.
struct UtdfNote {
    std::string code, node, text;
    bool operator==(const UtdfNote&) const = default;
};
// U2a (D443): a Node whose controller fails nemaCheck is left out unless `unsignalisedOnNemaFailure`
// writes it as unsignalised. `grid` maps world metres to the written X, Y (the desktop's UTM, U2b);
// empty writes world metres.
struct UtdfOptions {
    UtdfMode mode{UtdfMode::geometric};
    bool unsignalisedOnNemaFailure{};
    std::function<std::array<double, 2>(double, double)> grid{};
};
// `csv` is CRLF, without a BOM, C locale; empty when no Node can be written (UTDF_NOTHING_TO_EXPORT).
struct UtdfExport { std::string csv; std::vector<UtdfNote> report; };
UtdfExport utdfDocument(const ProjectDocument&, const UtdfOptions&);
UtdfExport utdfDocument(const ProjectDocument&, UtdfMode = UtdfMode::geometric);
}
