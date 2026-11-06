#pragma once
#include "trip_generation_panel.hpp"

namespace multra {
// M10.4 E2 (MODE_CHOICE §7, D479): self-owned Results page for mode choice, built as the Trip distribution
// page: choices of rate set, deterrence set, cost matrix, skim mode and mode-choice set; Run on a worker
// with its five stages; Mode choice sets…, Period factors…, and Write OD matrices (one History step that
// replaces only the generated matrices, enabled for a fresh result); one row per period, alternative and
// zone pair plus a totals row. A run is never a History step. It takes the Trip generation page's context.
class ModeChoicePanel final : public QWidget {
public:
    ModeChoicePanel(QWidget*, TripGenerationContext);
    ~ModeChoicePanel();
    void run();
    void editSets();
    void editFactors();
    void writeMatrices();
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
