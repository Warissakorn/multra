#pragma once
#include <filesystem>
#include <optional>
namespace multra {
// A content --data-dir override does not hide the database shipped with the executable.
std::optional<std::filesystem::path> projectionDataDirectory(const std::filesystem::path& content,
                                                            const std::filesystem::path& executableDirectory);
}
