#pragma once
// M10.3 D2 (D474): the owned-list dialog G2 built for trip-rate sets (D471), shared by trip-rate sets,
// deterrence sets and cost matrices: a table, Import (JSON), Export, Remove, Confirm/Cancel. Values are
// edited outside MULTRA. Local until accepted; the caller commits the list in one History step.
#include "od_lot_dialogs.hpp"
#include "trip_rate_dialog.hpp"
#include "../project/json.hpp"
#include <QStringList>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace multra {
struct OwnedListSpec {
    const char* dialogName;              // object names: the dialog, its table, buttons and error label
    const char* tableName;
    const char *importName, *exportName, *removeName, *errorName;
    const char* titleKey;                // locale keys
    std::vector<const char*> columnKeys;
    const char* invalidCode;             // shown when the file is not JSON
    const char* exportFailedKey;
    // One item's id and table cells; parse a file's JSON (throws ValidationError at "file…"); export.
    std::function<std::string(const Json&)> id;
    std::function<QStringList(const Json&)> cells;
    std::function<Json(const Json&)> parse;
    // M10.4 N1 (D481): an optional in-place edit of the selected row (the nests of a mode-choice set). Unset:
    // no button. The result replaces the row in the local list; nullopt (Cancel) changes nothing.
    const char* editName{};
    const char* editKey{};
    std::function<std::optional<Json>(QWidget*, const Json&)> edit;
};
// `items` are in their export form; the result is too, or nullopt on Cancel.
std::optional<std::vector<Json>> editOwnedList(QWidget*, const OwnedListSpec&, std::vector<Json> items, const DialogText&, const TripRateFiles&);
}
