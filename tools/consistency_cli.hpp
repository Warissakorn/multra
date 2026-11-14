// M12 R1 on the command line (docs/reference/SUBAREA_CONSISTENCY.md §7, D553).
#pragma once
#include <optional>

namespace multra {
// With --consistency-report OUT among the arguments: takes only --project, --seeds (default 42-46),
// --csv and --data-dir, refuses an existing OUT or CSV before any run, writes the report and returns 0.
// Without it: nullopt, and the caller parses the arguments as before.
std::optional<int> consistencyCommand(int argc, char** argv);
}
