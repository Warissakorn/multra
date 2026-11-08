#include "table_grid.hpp"
#include <QAbstractSpinBox>
#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QComboBox>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QKeyEvent>
#include <QLineEdit>
#include <QTableView>
#include <algorithm>
#include <set>
namespace multra {
namespace {
// Excel's tab-separated reading: a field holding a tab, line break or quote is quoted, with
// quotes doubled; everything else goes in as it is.
QString field(QString value) {
    if(!value.contains(QLatin1Char('\t'))&&!value.contains(QLatin1Char('\n'))&&!value.contains(QLatin1Char('\r'))&&!value.contains(QLatin1Char('"')))return value;
    value.replace(QLatin1String("\""),QLatin1String("\"\""));
    return QLatin1Char('"')+value+QLatin1Char('"');
}
// What the cell shows: an editor placed in the cell first, else the item's text, else its tick.
QString cellText(const QTableView* table,const QModelIndex& index) {
    if(auto* widget=table->indexWidget(index)) {
        if(auto* combo=qobject_cast<QComboBox*>(widget))return combo->currentText();
        if(auto* spin=qobject_cast<QAbstractSpinBox*>(widget))return spin->text();
        if(auto* line=qobject_cast<QLineEdit*>(widget))return line->text();
        if(auto* box=qobject_cast<QCheckBox*>(widget))return box->isChecked()?QStringLiteral("TRUE"):QStringLiteral("FALSE");
        if(auto* inner=widget->findChild<QComboBox*>())return inner->currentText();
        if(auto* inner=widget->findChild<QAbstractSpinBox*>())return inner->text();
        if(auto* inner=widget->findChild<QLineEdit*>())return inner->text();
    }
    const auto text=index.data(Qt::DisplayRole).toString();
    const auto check=index.data(Qt::CheckStateRole);
    if(text.isEmpty()&&check.isValid())return check.toInt()==Qt::Checked?QStringLiteral("TRUE"):QStringLiteral("FALSE");
    return text;
}
class SpreadsheetKeys final : public QObject {
public:
    using QObject::QObject;
    bool eventFilter(QObject* object,QEvent* event) override {
        auto* table=qobject_cast<QTableView*>(object);
        if(!table||(event->type()!=QEvent::ShortcutOverride&&event->type()!=QEvent::KeyPress))return false;
        auto* key=static_cast<QKeyEvent*>(event);
        const bool copy=key->matches(QKeySequence::Copy);
        // Select all only means something when more than one cell can be selected.
        const bool all=key->matches(QKeySequence::SelectAll)&&(table->selectionMode()==QAbstractItemView::ExtendedSelection||table->selectionMode()==QAbstractItemView::MultiSelection);
        if(!copy&&!all)return false;
        // Taking the override keeps a window shortcut on the same keys (Ctrl+A) from firing
        // while the table has focus; the key press then reaches the table.
        if(event->type()==QEvent::ShortcutOverride){event->accept();return true;}
        if(copy){QApplication::clipboard()->setText(tableSelectionText(table));return true;}
        return false; // the view's own handler selects all
    }
};
}
void makeSpreadsheetTable(QTableView* table) {
    if(table->property("editorSpreadsheet").toBool())return;
    table->setProperty("editorSpreadsheet",true);
    // A block of cells only where several things may be selected. A single-selection table picks
    // one object by its row, and Qt's selectRow() is a no-op there in cell mode.
    if(table->selectionMode()==QAbstractItemView::ExtendedSelection||table->selectionMode()==QAbstractItemView::MultiSelection)
        table->setSelectionBehavior(QAbstractItemView::SelectItems);
    table->verticalHeader()->setVisible(true);
    table->verticalHeader()->setDefaultAlignment(Qt::AlignCenter);
    table->setCornerButtonEnabled(true);
    static auto* keys=new SpreadsheetKeys(qApp);
    table->installEventFilter(keys);
}
std::vector<int> selectedTableRows(const QAbstractItemView* view) {
    std::set<int> rows;
    if(auto* selection=view->selectionModel())for(const auto& index:selection->selectedIndexes())rows.insert(index.row());
    return {rows.begin(),rows.end()};
}
QString tableSelectionText(const QTableView* table) {
    auto* model=table->model();auto* selection=table->selectionModel();
    if(!model||!selection)return {};
    auto indexes=selection->selectedIndexes();
    if(indexes.isEmpty()&&table->currentIndex().isValid())indexes={table->currentIndex()};
    indexes.removeIf([&](const QModelIndex& index){return table->isRowHidden(index.row())||table->isColumnHidden(index.column());});
    if(indexes.isEmpty())return {};
    if(indexes.size()==1)return field(cellText(table,indexes.front()));
    int top=model->rowCount(),bottom=-1,left=model->columnCount(),right=-1;
    std::set<std::pair<int,int>> chosen;
    for(const auto& index:indexes) {
        top=std::min(top,index.row());bottom=std::max(bottom,index.row());
        left=std::min(left,index.column());right=std::max(right,index.column());
        chosen.emplace(index.row(),index.column());
    }
    std::vector<int> rows,columns;
    for(int row=top;row<=bottom;++row)if(!table->isRowHidden(row))rows.push_back(row);
    for(int column=left;column<=right;++column)if(!table->isColumnHidden(column))columns.push_back(column);
    // Row labels count only when the table names its rows (an OD matrix's zones), not 1, 2, 3.
    bool named=false;
    for(int row:rows)if(model->headerData(row,Qt::Vertical).toString()!=QString::number(row+1))named=true;
    QStringList lines,header;
    if(named)header<<QString();
    for(int column:columns)header<<field(model->headerData(column,Qt::Horizontal).toString());
    lines<<header.join(QLatin1Char('\t'));
    for(int row:rows) {
        QStringList cells;
        if(named)cells<<field(model->headerData(row,Qt::Vertical).toString());
        for(int column:columns)cells<<(chosen.count({row,column})?field(cellText(table,model->index(row,column))):QString());
        lines<<cells.join(QLatin1Char('\t'));
    }
    return lines.join(QLatin1Char('\n'))+QLatin1Char('\n');
}
}
