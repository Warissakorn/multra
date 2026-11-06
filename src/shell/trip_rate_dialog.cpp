#include "trip_rate_dialog.hpp"
#include "owned_list_dialog.hpp"
#include <nlohmann/json.hpp>

namespace multra {
namespace {
QString str(const Json& j, const char* key) { return QString::fromStdString(j.at(key).get<std::string>()); }
QString purposeIds(const Json& j) {
    QStringList ids;
    for (const auto& p : j.at("purposes")) ids << QString::fromStdString(p.at("id").get<std::string>());
    return ids.join(", ");
}
template<class T, class Parse, class Write>
std::optional<std::vector<T>> run(QWidget* parent, OwnedListSpec spec, const std::vector<T>& current, Parse parse, Write write,
                                  const DialogText& text, const TripRateFiles& files) {
    spec.id = [](const Json& j) { return j.at("id").get<std::string>(); };
    spec.parse = [=](const Json& j) { return write(parse(j)); };
    std::vector<Json> items;
    for (const auto& x : current) items.push_back(write(x));
    const auto edited = editOwnedList(parent, spec, std::move(items), text, files);
    if (!edited) return std::nullopt;
    std::vector<T> out;
    for (const auto& j : *edited) out.push_back(parse(j));
    return out;
}
}
// M10.2 G2 (D471); since D2 (D474) a wrapper of the shared owned-list dialog with G2's object names.
std::optional<std::vector<TripRateSet>> editTripRateSets(QWidget* parent, const ProjectDocument& d, const DialogText& text,
                                                         const TripRateFiles& files) {
    OwnedListSpec spec{"editorTripRateSetsDialog", "editorTripRateSetsTable", "editorTripRateImport", "editorTripRateExport",
                       "editorTripRateRemove", "editorTripRateError", "editorTripRateSets",
                       {"editorColumnId", "editorColumnName", "editorTripRateUnit", "editorTripRatePurposes"},
                       "INVALID_TRIP_RATE_SET", "editorTripRateExportFailed", {}, {}, {}};
    spec.cells = [](const Json& j) { return QStringList{str(j, "id"), str(j, "name"), str(j, "unit"), purposeIds(j)}; };
    return run<TripRateSet>(parent, spec, d.tripRateSets, [](const Json& j) { return parseTripRateSet(j, "file", false); }, tripRateSetJson, text, files);
}
std::optional<std::vector<DeterrenceSet>> editDeterrenceSets(QWidget* parent, const ProjectDocument& d, const DialogText& text,
                                                             const TripRateFiles& files) {
    OwnedListSpec spec{"editorDeterrenceSetsDialog", "editorDeterrenceSetsTable", "editorDeterrenceImport", "editorDeterrenceExport",
                       "editorDeterrenceRemove", "editorDeterrenceError", "editorDeterrenceSets",
                       {"editorColumnId", "editorColumnName", "editorDeterrenceBetas"},
                       "INVALID_DETERRENCE_SET", "editorOwnedExportFailed", {}, {}, {}};
    spec.cells = [](const Json& j) {
        QStringList betas;
        for (const auto& p : j.at("purposes"))
            betas << QString::fromStdString(p.at("id").get<std::string>()) + " " + QString::number(p.at("beta").get<double>());
        return QStringList{str(j, "id"), str(j, "name"), betas.join(", ")};
    };
    return run<DeterrenceSet>(parent, spec, d.deterrenceSets, [](const Json& j) { return parseDeterrenceSet(j, "file", false); },
                              deterrenceSetJson, text, files);
}
std::optional<std::vector<CostMatrix>> editCostMatrices(QWidget* parent, const ProjectDocument& d, const DialogText& text,
                                                        const TripRateFiles& files) {
    OwnedListSpec spec{"editorCostMatricesDialog", "editorCostMatricesTable", "editorCostMatrixImport", "editorCostMatrixExport",
                       "editorCostMatrixRemove", "editorCostMatrixError", "editorCostMatrices",
                       {"editorColumnId", "editorColumnName", "editorCostMatrixZones"},
                       "INVALID_COST_MATRIX", "editorOwnedExportFailed", {}, {}, {}};
    spec.cells = [](const Json& j) { return QStringList{str(j, "id"), str(j, "name"), QString::number(j.at("zoneIds").size())}; };
    return run<CostMatrix>(parent, spec, d.costMatrices, [](const Json& j) { return parseCostMatrix(j, "file"); }, costMatrixJson, text, files);
}
}
