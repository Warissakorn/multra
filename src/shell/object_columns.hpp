#pragma once
#include "../project/document.hpp"
#include <QString>
#include <QStringList>
#include <cstddef>
#include <functional>
#include <string>
#include <vector>

namespace multra {
// D525 (docs/plans/OBJECT_TABLE.md): an object type's columns, which one ObjectTable shows for every
// type, as Vissim's lists and SUMO's inspect frame do.
enum class CellKind { id, text, number }; // id, number: tabular digits; number right-aligned
template <class T> struct ObjectColumn {
    std::string key;    // stable attribute name ("name", "start"); the column's identity
    std::string header; // locale key of the header
    CellKind kind{CellKind::text};
    std::function<QString(const T&)> display;
    // Empty: read-only. Otherwise an in-place edit: one History step named `undo`; it may throw (a
    // parse or validation refusal), and the cell then shows the stored value again.
    std::function<void(ProjectDocument&, const std::string& id, const QString&)> edit{};
    std::string undo{};
};
template <class T> struct ObjectTableSpec {
    std::function<const std::vector<T>&(const ProjectDocument&)> objects;
    std::function<std::string(const T&)> id;
    std::vector<ObjectColumn<T>> columns;
};

// The untyped form ObjectTable works on: a column's identity, header, kind and edit; one row's id
// and cell texts.
struct TableColumn {
    std::string key, header;
    CellKind kind{CellKind::text};
    std::function<void(ProjectDocument&, const std::string& id, const QString&)> edit{};
    std::string undo{};
};
struct TableRow { std::string id; QStringList cells; };
struct ObjectTableModel {
    std::vector<TableColumn> columns;
    std::function<std::size_t(const ProjectDocument&)> count; // cheap: read on every refresh
    std::function<std::vector<TableRow>(const ProjectDocument&)> rows; // only when rebuilding
};
template <class T> ObjectTableModel objectTableModel(ObjectTableSpec<T> spec) {
    ObjectTableModel model;
    for (const auto& c : spec.columns) model.columns.push_back({c.key, c.header, c.kind, c.edit, c.undo});
    model.count = [spec](const ProjectDocument& d) { return spec.objects(d).size(); };
    model.rows = [spec](const ProjectDocument& d) {
        std::vector<TableRow> rows;
        for (const auto& object : spec.objects(d)) {
            TableRow row{spec.id(object), {}};
            for (const auto& c : spec.columns) row.cells << c.display(object);
            rows.push_back(std::move(row));
        }
        return rows;
    };
    return model;
}
}
