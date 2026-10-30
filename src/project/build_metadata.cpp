#include "build_metadata.hpp"
#include "build_info.hpp"
#include <nlohmann/json.hpp>
namespace multra {
void addBuildMetadata(Json& result) {
    result["engineVersion"] = std::string(MULTRA_VERSION) + "-cpp-m0";
    result["buildCommit"] = MULTRA_BUILD_COMMIT;
    result["compiler"] = MULTRA_COMPILER;
}
}
