#pragma once
class QTableWidget;
class QTabWidget;
namespace multra {
// Size initial object columns once, cap long prose, then retain user-resized widths on refresh.
void fitTableColumns(QTableWidget*);
void sizeTabPages(QTabWidget*);
}
