#pragma once
#include "trip_generation_panel.hpp"

namespace multra {
// M9 A4 (DYNAMIC_ASSIGNMENT §17, D552): the self-owned *Dynamic assignment* Results page at the Micro level. The
// level (Micro or Meso), iteration cap, gap and seed; Run on a worker with Cancel between iterations; a note, the
// delta per iteration and one row per decision, cell and entry. The run never changes the document: *Apply weights*
// writes the shares into the decisions' flows as one History step while the result is current, and *Save route
// costs…* writes the MultraRouteCosts file. It takes the Trip generation context; `files.save` asks for the file.
class DynamicAssignmentPanel final : public QWidget {
public:
    DynamicAssignmentPanel(QWidget*, TripGenerationContext);
    ~DynamicAssignmentPanel();
    void run();
    void cancel();
    void apply();
    void save();
    void refresh();
    void translate();
    bool running() const;
    bool exportable() const;
    bool stale() const;
    std::string csv() const;
private:
    struct Data; std::unique_ptr<Data> d_;
};
}
