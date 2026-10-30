#pragma once
#include "demand_catalog_dialog.hpp"
#include "../project/document.hpp"
#include <QWidget>
#include <memory>
namespace multra {
struct NodeAnalysisContext {
    std::function<const ProjectDocument&()> document;
    std::function<std::uint64_t()> generation;
    std::function<QString()> file;
    std::filesystem::path data;
    CatalogText text;
};
// Self-owned Results page; no EditorWindow state or method is added for a dialog.
class NodeAnalysisPanel final:public QWidget {
public:
    NodeAnalysisPanel(QWidget*,NodeAnalysisContext);
    ~NodeAnalysisPanel();
    void analyse();
    void refresh();
    void translate();
    bool hasResult() const;
    std::string csv() const;
    std::string json() const; // full CLI-equivalent bytes, including source/build provenance
private:
    struct Data;std::unique_ptr<Data> d_;
};
}
