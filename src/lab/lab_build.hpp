#pragma once
// D166: what the two lab layouts share. Internal to src/lab/.
#include "lab_network.hpp"
#include <utility>

namespace multra::lab::build {
// A polyline drawn piece by piece; `at()` is the arclength so far, so gaps can be marked as drawn.
class Path {
public:
    explicit Path(Point start) : points_{start} {}
    void line(Point to);
    void arc(Point centre, double radius, double from, double to); // radians, sampled every ~3 degrees
    double at() const { return length_; }
    const std::vector<Point>& points() const { return points_; }
private:
    std::vector<Point> points_;
    double length_{};
};
// A closed loop cut at `gaps` (stations along `loop`, ascending, not wrapping): one Link between
// consecutive gaps and one Connector across each gap. `objects` is the Route order, starting with
// the Link after the last gap; `into[k]` is the Link that ends at gap k.
struct Loop { std::vector<std::string> objects, into; };
Loop addLoop(ProjectDocument&, const std::vector<Point>& loop, const std::vector<std::pair<double, double>>& gaps,
             int lanes, double width, const std::string& name);
// One fixed-time controller with the given groups and a head on every lane of each approach Link,
// 1 m before its end (the four-leg fixture's placement).
void signalise(ProjectDocument&, const std::string& name, double offset, const RoadSettings&,
               const std::vector<std::string>& mainApproaches, const std::vector<std::string>& crossApproaches);
std::string firstLane(const ProjectDocument&, const std::string& link);
// The lab's own types and behaviours, as an owned catalog, and the long run duration.
void finishDefinition(ProjectDocument&, const LabSettings&);
LabNetwork oval(const LabSettings&);
LabNetwork urban(const LabSettings&);
}
