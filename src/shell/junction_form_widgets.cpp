#include "junction_form_widgets.hpp"
#include <QCheckBox>
#include <QComboBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>
#include <cmath>
#include <stdexcept>
#include <algorithm>
namespace multra {
JunctionLaneWidgets junctionLaneWidgets(QWidget* parent,const std::vector<JunctionLane>& lanes,DrivingSide side,const QString& name,const CatalogText& text,const std::function<void()>& changed) {
    auto* body=new QWidget(parent);auto* layout=new QVBoxLayout(body);layout->setContentsMargins(0,0,0,0);
    auto* table=new QTableWidget(0,3,body);table->setObjectName(name);table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setHorizontalHeaderLabels({text("editorJunctionMovements"),text("editorJunctionPocket"),text("editorJunctionPocketSide")});
    table->horizontalHeader()->setStretchLastSection(true);table->setMinimumHeight(140);layout->addWidget(table);
    const auto add=[=](const JunctionLane& lane) {
        const int row=table->rowCount();table->insertRow(row);
        auto* moves=new QComboBox(table);moves->addItem(text("editorJunctionSelect"),QString());
        for(const auto* key:{"F","T","K","FT","TK","FTK"}) {
            QStringList names;for(const char m:std::string(key))names<<text(m=='T'?"editorJunctionThrough":(m=='F')==(side==DrivingSide::left)?"editorJunctionRight":"editorJunctionLeft");
            moves->addItem(QString::fromUtf8(key)+" — "+names.join(" / "),QString::fromUtf8(key));
        }
        moves->setCurrentIndex(std::max(0,moves->findData(QString::fromStdString(lane.movements))));
        auto* pocket=new QCheckBox(table);pocket->setChecked(lane.pocket);
        auto* edge=new QComboBox(table);edge->addItem(text("editorJunctionAutomatic"),QString());edge->addItem(text("editorJunctionFar"),"far");edge->addItem(text("editorJunctionKerb"),"kerb");
        edge->setCurrentIndex(!lane.side?0:*lane.side==PocketSide::far?1:2);
        table->setCellWidget(row,0,moves);table->setCellWidget(row,1,pocket);table->setCellWidget(row,2,edge);
        QObject::connect(moves,&QComboBox::currentIndexChanged,body,[=]{changed();});QObject::connect(pocket,&QCheckBox::toggled,body,[=]{changed();});QObject::connect(edge,&QComboBox::currentIndexChanged,body,[=]{changed();});
        table->setCurrentCell(row,0);table->resizeColumnsToContents();
    };
    for(const auto& lane:lanes)add(lane);
    auto* buttons=new QHBoxLayout;layout->addLayout(buttons);auto* plus=new QPushButton(text("editorJunctionAddLane"),body);plus->setObjectName(name+"Add");auto* minus=new QPushButton(text("editorJunctionRemoveLane"),body);minus->setObjectName(name+"Remove");buttons->addWidget(plus);buttons->addWidget(minus);buttons->addStretch();
    QObject::connect(plus,&QPushButton::clicked,body,[=]{add({});changed();});QObject::connect(minus,&QPushButton::clicked,body,[=]{if(table->currentRow()>=0)table->removeRow(table->currentRow());changed();});
    return {body,table};
}
std::vector<JunctionLane> junctionLaneValues(const JunctionLaneWidgets& w) {
    std::vector<JunctionLane> out;
    for(int row=0;row<w.table->rowCount();++row) {
        JunctionLane lane;lane.movements=qobject_cast<QComboBox*>(w.table->cellWidget(row,0))->currentData().toString().toStdString();lane.pocket=qobject_cast<QCheckBox*>(w.table->cellWidget(row,1))->isChecked();
        const auto side=qobject_cast<QComboBox*>(w.table->cellWidget(row,2))->currentData().toString();if(!side.isEmpty())lane.side=side=="far"?PocketSide::far:PocketSide::kerb;out.push_back(std::move(lane));
    }return out;
}
QLineEdit* junctionNumber(QWidget* parent,const std::optional<double>& value,const char* name) {
    auto* field=new QLineEdit(parent);field->setObjectName(name);if(value)field->setText(QString::number(*value,'g',17));return field;
}
std::optional<double> junctionNumberValue(const QLineEdit* field,bool zeroAllowed) {
    if(field->text().trimmed().isEmpty())return std::nullopt;
    bool ok=false;const double value=field->text().toDouble(&ok);
    if(!ok||!std::isfinite(value)||(zeroAllowed?value<0:value<=0))throw std::invalid_argument("EDIT_NODE_INVALID");
    return value;
}
QString junctionIssueText(const std::vector<ValidationIssue>& issues,const CatalogText& text) {
    QStringList out;for(const auto& i:issues)out<<QString::fromStdString(i.code)+" — "+text(i.code.c_str())+" — "+QString::fromStdString(i.path);return out.join('\n');
}
}
