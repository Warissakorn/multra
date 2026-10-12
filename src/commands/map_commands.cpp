#include "map_commands.hpp"
namespace trafficsim {
void setMapSettings(ProjectDocument& document, std::optional<MapSettings> settings) {
    if (settings) validateMapSettings(*settings);
    document.map = std::move(settings);
}
}
