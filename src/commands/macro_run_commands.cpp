#include "macro_run_commands.hpp"

namespace multra {
void setMacroRunDefinitions(ProjectDocument& d, std::vector<MacroRunDefinition> definitions) {
    // Reserve every supplied id before allocating empty ones: a later row may already own the
    // next generated id. The old list also remains reserved, so removed ids are not recycled.
    auto reserved = d;
    reserved.macroRuns.insert(reserved.macroRuns.end(), definitions.begin(), definitions.end());
    for (auto& definition : definitions) {
        if (definition.id.empty()) {
            definition.id = allocateId(reserved, "macro-run");
            reserved.macroRuns.push_back(definition);
        }
    }
    d.nextId = reserved.nextId;
    d.macroRuns = std::move(definitions);
}
}
