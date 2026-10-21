#pragma once
// Plane polygon tests shared by zones (ZONES_AND_PARKING §2) and Nodes (NODES §2). A polygon is a
// ring of points; the closing edge is implied.
#include "network.hpp"
#include <vector>

namespace multra {
// Whether segments ab and cd touch, end points and collinear overlap included.
bool segmentsMeet(Point a, Point b, Point c, Point d);
// At least three finite points, and no two non-adjacent edges touch (the ring is simple).
bool simplePolygon(const std::vector<Point>& polygon);
// Ray cast; a point on the boundary counts as inside.
bool pointInPolygon(const std::vector<Point>& polygon, Point p);
// Two simple polygons share any point: an edge of one touches an edge of the other, or one holds
// a vertex of the other.
bool polygonsOverlap(const std::vector<Point>& a, const std::vector<Point>& b);
}
