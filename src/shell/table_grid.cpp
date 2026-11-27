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
#include <QPainter>
#include <QScrollBar>
#include <QStyledItemDelegate>
#include <QPointer>
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
// D605: drawn over the cells (above the view's own paint), never taking a click.
class SelectionOutline final : public QWidget {
public:
    // A child of the table over its viewport, not of the viewport: the view scrolls the viewport's
    // children with its pixels.
    explicit SelectionOutline(QTableView* table):QWidget(table),table_(table) {
        setAttribute(Qt::WA_TransparentForMouseEvents);setAttribute(Qt::WA_NoSystemBackground);
        setObjectName(QStringLiteral("editorTableSelectionOutline"));
        table->viewport()->installEventFilter(this);
        setGeometry(table->viewport()->geometry());raise();show();
        if((watched_=table->selectionModel()))connect(watched_,&QItemSelectionModel::selectionChanged,this,[this]{update();});
        const auto redraw=[this]{update();};
        connect(table->horizontalScrollBar(),&QScrollBar::valueChanged,this,redraw);
        connect(table->verticalScrollBar(),&QScrollBar::valueChanged,this,redraw);
        connect(table->horizontalHeader(),&QHeaderView::sectionResized,this,redraw);
        connect(table->verticalHeader(),&QHeaderView::sectionResized,this,redraw);
    }
    bool eventFilter(QObject*,QEvent* event) override {
        if(event->type()==QEvent::Resize||event->type()==QEvent::Move){setGeometry(table_->viewport()->geometry());raise();}
        // A selection change redraws the whole overlay: a block's outline moves off cells the view
        // does not repaint. The model (and so the selection model) can be replaced; follow it here.
        if(event->type()==QEvent::Paint&&table_&&table_->selectionModel()!=watched_) {
            watched_=table_->selectionModel();
            if(watched_)connect(watched_,&QItemSelectionModel::selectionChanged,this,[this]{update();});
        }
        return false;
    }
protected:
    void paintEvent(QPaintEvent*) override {
        if(!table_||!table_->selectionModel())return;
        QPainter painter(this);painter.setClipRect(rect());
        auto tint=palette().color(QPalette::Highlight);tint.setAlpha(48);
        for(const auto& index:table_->selectionModel()->selectedIndexes())
            if(!table_->isRowHidden(index.row())&&!table_->isColumnHidden(index.column()))
                painter.fillRect(table_->visualRect(index),tint);
        QPen pen(palette().color(QPalette::Highlight),2);pen.setCapStyle(Qt::SquareCap);
        painter.setPen(pen);
        for(const auto& line:tableSelectionOutline(table_))painter.drawLine(line);
    }
private:
    QPointer<QTableView> table_;QPointer<QItemSelectionModel> watched_;
};
}
// F8 (owner, 2026-11-26): a cell's editor covers its cell exactly. Qt places it on the item's text
// rect, which the item padding of the style sheet insets, and the field rule pins its height at
// 24 px; the editor then sat 4 px right and 2 px down of a 23 px cell and spilled past it. The
// property lets the style sheet drop the field's pinned height, padding and radius for it.
class CellEditors:public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;
    QWidget* createEditor(QWidget* parent,const QStyleOptionViewItem& option,const QModelIndex& index) const override {
        auto* editor=QStyledItemDelegate::createEditor(parent,option,index);
        if(editor)editor->setProperty("cellEditor",true);
        return editor;
    }
    void updateEditorGeometry(QWidget* editor,const QStyleOptionViewItem& option,const QModelIndex&) const override { editor->setGeometry(option.rect); }
};
void makeSpreadsheetTable(QTableView* table) {
    if(table->property("editorSpreadsheet").toBool())return;
    table->setProperty("editorSpreadsheet",true);
    // Only Qt's own delegate is replaced; a table with a delegate of its own keeps it.
    if(table->itemDelegate()&&table->itemDelegate()->metaObject()==&QStyledItemDelegate::staticMetaObject)
        table->setItemDelegate(new CellEditors(table));
    // A block of cells only where several things may be selected. A single-selection table picks
    // one object by its row, and Qt's selectRow() is a no-op there in cell mode.
    if(table->selectionMode()==QAbstractItemView::ExtendedSelection||table->selectionMode()==QAbstractItemView::MultiSelection) {
        table->setSelectionBehavior(QAbstractItemView::SelectItems);
        new SelectionOutline(table);
    }
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
std::vector<QLine> tableSelectionOutline(const QTableView* table) {
    std::vector<QLine> lines;
    auto* model=table->model();auto* selection=table->selectionModel();
    if(!model||!selection)return lines;
    // The next shown row or column in a direction, or -1 past the edge.
    const auto nextRow=[&](int row,int step){for(row+=step;row>=0&&row<model->rowCount();row+=step)if(!table->isRowHidden(row))return row;return -1;};
    const auto nextColumn=[&](int column,int step){for(column+=step;column>=0&&column<model->columnCount();column+=step)if(!table->isColumnHidden(column))return column;return -1;};
    const auto chosen=[&](int row,int column){return row>=0&&column>=0&&selection->isSelected(model->index(row,column));};
    for(const auto& index:selection->selectedIndexes()) {
        const int r=index.row(),c=index.column();
        if(table->isRowHidden(r)||table->isColumnHidden(c))continue;
        const auto rect=table->visualRect(index);
        if(rect.isEmpty())continue;
        if(!chosen(nextRow(r,-1),c))lines.emplace_back(rect.topLeft(),rect.topRight());
        if(!chosen(nextRow(r,1),c))lines.emplace_back(rect.bottomLeft(),rect.bottomRight());
        if(!chosen(r,nextColumn(c,-1)))lines.emplace_back(rect.topLeft(),rect.bottomLeft());
        if(!chosen(r,nextColumn(c,1)))lines.emplace_back(rect.topRight(),rect.bottomRight());
    }
    return lines;
}
}
