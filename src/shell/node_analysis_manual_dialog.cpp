#include "node_analysis_manual_dialog.hpp"
#include "dialog_fit.hpp"
#include "editor_interaction.hpp"
#include "../editor/ui_design_tokens.hpp"
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <algorithm>
namespace multra {
std::optional<NodeAnalysisManualWindow> editNodeManualWindow(QWidget* parent,const NodeAnalysisReport& analysis,
    const NodeAnalysisMicroSource& source,const BatchReport& batch,const MovementReport& evaluation,
    const std::vector<NodeAnalysisManualWindow>& confirmed,const CatalogText& text) {
    QDialog dialog(parent);dialog.setObjectName("editorAnalysisManualDialog");dialog.setWindowTitle(text("editorAnalysisManualWindow"));
    auto* body=scrollingBody(dialog);auto* box=new QVBoxLayout(body);
    auto* note=new ReflowLabel(body);note->setWordWrap(true);note->setTextFormat(Qt::PlainText);box->addWidget(note);
    auto* form=new QFormLayout;form->setRowWrapPolicy(QFormLayout::WrapLongRows);box->addLayout(form);
    auto* node=new QComboBox(body);node->setObjectName("editorAnalysisManualNode");
    for(const auto& n:analysis.run.nodes)if(!n.modelSource)node->addItem(QString::fromStdString(n.name.empty()?n.id:n.name+" / "+n.id),QString::fromStdString(n.id));
    if(node->count()==0)return {};
    form->addRow(text("editorAnalysisNode"),node);
    const auto field=[&](const char* key){auto* w=new QLineEdit(body);w->setObjectName(key);w->setFont(editorDesign::numericFont());form->addRow(text(key),w);return w;};
    auto* start=field("editorAnalysisManualStart");auto* end=field("editorAnalysisManualEnd");
    auto* status=new ReflowLabel(body);status->setObjectName("editorAnalysisManualIssues");status->setTextFormat(Qt::PlainText);status->setWordWrap(true);box->addWidget(status);
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&dialog);addFooter(dialog,buttons);
    buttons->button(QDialogButtonBox::Ok)->setText(text("editorConfirm"));buttons->button(QDialogButtonBox::Cancel)->setText(text("editorCancel"));
    std::optional<NodeAnalysisManualWindow> result;bool selecting=false;
    const auto validate=[&]{
        if(selecting)return;
        result.reset();buttons->button(QDialogButtonBox::Ok)->setEnabled(false);const auto id=node->currentData().toString().toStdString();
        bool validStart=false,validEnd=false;const double from=start->text().toDouble(&validStart),to=end->text().toDouble(&validEnd);
        if(!validStart||!validEnd){status->setText(text("NODE_MICRO_NO_WINDOW")+" ("+QString::fromStdString(id)+")");return;}
        auto candidates=confirmed;std::erase_if(candidates,[&](const auto& w){return w.nodeId==id;});candidates.push_back({id,from,to});
        const auto preview=compareNodeAnalysis(analysis,source,batch,evaluation,candidates);
        if(std::any_of(preview.rows.begin(),preview.rows.end(),[&](const auto& row){return row.nodeId==id;})) {
            result=NodeAnalysisManualWindow{id,from,to};status->clear();buttons->button(QDialogButtonBox::Ok)->setEnabled(true);
        } else {
            const auto issue=std::find_if(preview.issues.begin(),preview.issues.end(),[&](const auto& i){return i.path==id;});
            const auto& refused=issue!=preview.issues.end()?*issue:preview.issues.front();status->setText(text(refused.code.c_str())+" ("+QString::fromStdString(refused.path)+")");
        }
    };
    const auto select=[&]{
        selecting=true;const auto id=node->currentData().toString().toStdString();
        const auto n=std::find_if(analysis.run.nodes.begin(),analysis.run.nodes.end(),[&](const auto& n){return n.id==id;});
        note->setText(text("editorAnalysisManualNote").arg(QString::number(evaluation.warmup,'g',12),QString::number(evaluation.evaluationEnd,'g',12),QString::number(n->analysisPeriod,'g',12)));
        const auto previous=std::find_if(confirmed.begin(),confirmed.end(),[&](const auto& w){return w.nodeId==id;});
        start->setText(previous==confirmed.end()?QString():QString::number(previous->start,'g',17));end->setText(previous==confirmed.end()?QString():QString::number(previous->end,'g',17));
        selecting=false;validate();
    };
    QObject::connect(start,&QLineEdit::textChanged,&dialog,validate);QObject::connect(end,&QLineEdit::textChanged,&dialog,validate);
    QObject::connect(node,&QComboBox::currentIndexChanged,&dialog,select);
    QObject::connect(buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);QObject::connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    select();dialog.resize(640,360);if(dialog.exec()!=QDialog::Accepted)return {};return result;
}
}
