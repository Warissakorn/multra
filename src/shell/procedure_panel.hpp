#pragma once
#include "trip_generation_panel.hpp"

namespace multra {
// Procedure sequence P2 (PROCEDURE_SEQUENCE §4, D485): self-owned *Procedure* Results page. A procedure choice,
// Run on a worker with Cancel (between steps and inside an assignment), Procedures…, a note with the outcome
// and one row per step (step, kind, status, summary). A run never changes the document and is no History
// step; an edit after it marks it stale. CSV: the step table, marked. It takes the Trip generation context.
class ProcedurePanel final : public QWidget {
public:
    ProcedurePanel(QWidget*, TripGenerationContext);
    ~ProcedurePanel();
    void run();
    void cancel();
    void editProcedures();
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
