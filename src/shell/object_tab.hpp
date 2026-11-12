#pragma once
#include <cstdint>

class QTableWidget;
namespace multra {
// D520: one object tab's table and the History revision its rows were last built at (UINT64_MAX:
// stale, rebuilt on the next refresh). Thirteen EditorWindow tabs held this pair as two members
// each; their build/refresh/edit functions stay window members, each with its own refresh test.
struct ObjectTab {
    QTableWidget* table{};
    std::uint64_t revision{UINT64_MAX};
    void invalidate() { revision = UINT64_MAX; }
};
}
