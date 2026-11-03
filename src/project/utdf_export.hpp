#pragma once
// UTDF U1 (UTDF_EXPORT §7 U1 contract, D437, D440, D442): the UTDF 8 combined CSV's geometry
// sections, [Network], [Nodes], [Links] and [Lanes], for Synchro 12 and Vistro 2026. Pure: reads the
// document and never changes it. Volumes, timing, UTM and the Vistro median patch are U2.
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
// `csv` is CRLF, without a BOM, C locale; empty when no Node can be written (UTDF_NOTHING_TO_EXPORT).
struct UtdfExport { std::string csv; std::vector<UtdfNote> report; };
UtdfExport utdfDocument(const ProjectDocument&, UtdfMode = UtdfMode::geometric);
}
