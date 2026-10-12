#include "projection_data.hpp"
#include <array>
namespace trafficsim {
std::optional<std::filesystem::path> projectionDataDirectory(const std::filesystem::path& content,
                                                            const std::filesystem::path& executable) {
    for(const auto& directory:std::array<std::filesystem::path,2>{content/"proj",executable/"data/proj"}) {
        std::error_code error;if(std::filesystem::is_regular_file(directory/"proj.db",error))return directory;
    }
    return {};
}
}
