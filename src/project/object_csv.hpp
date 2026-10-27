#pragma once
#include <string>
namespace multra {
struct ProjectDocument;
enum class ObjectTable { links,connectors,inputs,routes,decisions };
const char* objectTableName(ObjectTable);
// One authored object per row, in document order. UTF-8 BOM, CRLF, quoted text/JSON cells.
// No compilation, catalog resolution or document mutation. Missing values are empty cells.
std::string objectTableCsv(const ProjectDocument&,ObjectTable);
}
