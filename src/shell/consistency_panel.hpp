#pragma once
#include "trip_generation_panel.hpp"
#include "../project/study_area_run.hpp"

namespace multra {
// M12 R2 (SUBAREA_CONSISTENCY §13, D554): the self-owned *Consistency* Results page at the Cross-level level. Seeds;
// Run on a worker with Cancel before Meso and between seeds; a note and one row per (period, gate or kept Link) with
// Macro, Meso and Micro veh/h and Δ, % and GEH per pair. No colours, no pass/fail; the run never changes the
// document. It takes the Trip generation context, and `sources` gives the side files beside the project.
class ConsistencyPanel final : public QWidget {
public:
    ConsistencyPanel(QWidget*, TripGenerationContext, std::function<StudyAreaSources()> sources);
    ~ConsistencyPanel();
    void run();
    void cancel();
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
