#include "junction_preview_dialog.hpp"
#include "dialog_fit.hpp"
#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>
namespace multra {
bool confirmJunctionEdit(QWidget* parent,const JunctionEditPreview& p,const CatalogText& text) {
    QDialog dialog(parent);dialog.setObjectName("editorJunctionPreviewDialog");dialog.setWindowTitle(text("editorJunctionAffected"));
    auto* body=scrollingBody(dialog);auto* layout=new QVBoxLayout(body);
    auto* count=new QLabel(text("editorJunctionObjectCount").arg(p.nodes.size()).arg(p.objects.size()),body);count->setObjectName("editorJunctionObjectCount");layout->addWidget(count);
    auto* table=new QTableWidget(static_cast<int>(p.nodes.size()),2,body);table->setObjectName("editorJunctionPreviewTable");
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);table->setHorizontalHeaderLabels({text("editorColumnName"),text("editorColumnId")});
    int row=0;for(const auto& id:p.nodes){table->setItem(row,0,new QTableWidgetItem(text("editorNodeTable")));table->setItem(row++,1,new QTableWidgetItem(QString::fromStdString(id)));}
    table->resizeColumnsToContents();layout->addWidget(table);
    auto* show=new QPushButton(text("editorJunctionShowDetail"),body);show->setObjectName("editorJunctionShowDetail");show->setCheckable(true);layout->addWidget(show);
    auto* detail=new QTableWidget(static_cast<int>(p.objects.size()),1,body);detail->setObjectName("editorJunctionDetailTable");detail->setEditTriggers(QAbstractItemView::NoEditTriggers);detail->setHorizontalHeaderLabels({text("editorColumnId")});
    row=0;for(const auto& id:p.objects)detail->setItem(row++,0,new QTableWidgetItem(QString::fromStdString(id)));detail->resizeColumnsToContents();layout->addWidget(detail);detail->hide();
    QObject::connect(show,&QPushButton::toggled,detail,&QWidget::setVisible);
    auto* issues=new QLabel(body);issues->setObjectName("editorJunctionPreviewIssues");issues->setWordWrap(true);QStringList reasons;
    for(const auto& issue:p.issues)reasons<<text(issue.code.c_str())+" — "+QString::fromStdString(issue.path);
    issues->setText(reasons.join('\n'));layout->addWidget(issues);
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&dialog);addFooter(dialog,buttons);
    buttons->button(QDialogButtonBox::Ok)->setText(text("editorJunctionReplace"));buttons->button(QDialogButtonBox::Ok)->setEnabled(p.issues.empty());
    buttons->button(QDialogButtonBox::Cancel)->setText(text("editorCancel"));
    QObject::connect(buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);QObject::connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    dialog.resize(680,440);return dialog.exec()==QDialog::Accepted;
}
}
