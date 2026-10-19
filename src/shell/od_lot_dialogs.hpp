#pragma once
#include "../model/demand/zones.hpp"
#include <QString>
#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <vector>
class QWidget;
namespace trafficsim {
// M8.6b (D162, docs/reference/ZONES_AND_PARKING.md §9). Local dialog values only: the caller commits
// the result with its usual history command, and the structural tier has the last word.
using DialogText = std::function<QString(const char*)>;
using Choice = std::pair<std::string, QString>; // id, label

// An OD matrix: period, composition, which zones (in `zones` order) and the zones x zones trips.
// Refuses locally an empty zone set (OD_ZONE_UNKNOWN) and end <= start (OD_PERIOD_INVALID).
// S1.1 (STUDY_PERIODS §8): `periods` offers a StudyPeriod in place of the own start and end.
std::optional<OdMatrix> editOdMatrix(QWidget*, const OdMatrix&, const std::vector<Choice>& zones,
                                     const std::vector<Choice>& compositions, const std::vector<Choice>& periods,
                                     const DialogText&);

// The lot of one zone. nullopt: cancelled; an engaged empty optional: "not a lot" (delete it).
std::optional<std::optional<ParkingLot>> editParkingLot(QWidget*, const std::optional<ParkingLot>& current,
                                                        const std::string& zoneId,
                                                        const std::vector<Choice>& compositions, const DialogText&);
}
