#pragma once
#include "od_lot_dialogs.hpp"
#include "../project/document.hpp"
#include <functional>

namespace multra {
// M10.2 G2 (TRIP_GENERATION §6.3, D471): where Import reads and Export writes. The window passes
// file dialogs; tests pass fixed paths. An empty path cancels.
struct TripRateFiles {
    std::function<QString()> open;
    std::function<QString(const QString& suggested)> save;
};
// The project's own trip-rate sets: a list with Import (JSON), Export and Remove. Values are
// edited outside MULTRA. Local until accepted; the caller commits the list in one History step.
std::optional<std::vector<TripRateSet>> editTripRateSets(QWidget*, const ProjectDocument&, const DialogText&, const TripRateFiles&);
}
