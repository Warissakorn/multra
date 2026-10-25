#pragma once
#include <QPainterPath>
#include <string_view>

namespace multra {
// Plan view in metres: nose at x=0, body along -x, centred on y=0. Decoration is
// separate from the solid body so glass/cargo never punches holes in the hit shape.
struct VehicleDrawing {
    QPainterPath body, glass, panels, lamps;
    void translate(double x, double y);
};
// Archetypes come from appearance data, independently of type IDs and physics.
// Omission preserves the legacy car/long-vehicle choice. Small symbols keep their
// silhouette but omit interior detail; the caller owns the pixel floor.
VehicleDrawing vehicleDrawing(double length, double width, bool detailed,
                              std::string_view archetype = {});
QPainterPath vehicleShape(double length, double width, bool detailed,
                         std::string_view archetype = {});
}
