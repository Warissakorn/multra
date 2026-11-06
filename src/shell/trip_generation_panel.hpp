#pragma once
#include "od_lot_dialogs.hpp"
#include "trip_rate_dialog.hpp"
#include "../project/document.hpp"
#include <QWidget>
#include <filesystem>
#include <functional>
#include <memory>

namespace multra {
// M10.2 G2 (TRIP_GENERATION §8, D471): what the Trip generation page needs from the window.
struct TripGenerationContext {
    std::function<const ProjectDocument&()> document;
    std::function<std::uint64_t()> generation; // History generation: New/Open discards a result
    std::filesystem::path data;
    DialogText text;
    // Commits one History step; false when it was refused (the window shows why).
    std::function<bool(const char* name, std::function<void(ProjectDocument&)>)> execute;
    TripRateFiles files;
    std::function<void()> presentationChanged; // Copy/Export enabled state
};
// Self-owned Results page: a rate-set choice, Run, Trip-rate sets…, the stages and one row per
// purpose and zone plus a totals row. Runs on a worker; never a History step. No EditorWindow member.
class TripGenerationPanel final : public QWidget {
public:
    TripGenerationPanel(QWidget*, TripGenerationContext);
    ~TripGenerationPanel();
    void run();          // with the chosen set
    void editSets();     // the Trip-rate sets… dialog
    void refresh();
    void translate();
    bool running() const;
    bool exportable() const;
    bool stale() const;
    std::string csv() const;  // marked; stale results say so
private:
    struct Data; std::unique_ptr<Data> d_;
};
}
