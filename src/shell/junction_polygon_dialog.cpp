#include "junction_polygon_dialog.hpp"
#include "dialog_fit.hpp"
#include <QDialog>
#include <QDialogButtonBox>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
namespace multra {
namespace {
class PolygonView final:public QGraphicsView {
public:
    PolygonView(const JunctionPolygonPreview& p,QWidget* parent):QGraphicsView(parent) {
        setObjectName("editorJunctionPolygonView");setScene(new QGraphicsScene(this));
        setRenderHint(QPainter::Antialiasing);setInteractive(false);setMinimumSize(280,220);
        const auto outline=[&](const auto& vertices,const QPen& pen) {
            QPolygonF polygon;for(const auto v:vertices)polygon<<QPointF(v.x,-v.y);
            scene()->addPolygon(polygon,pen);
        };
        outline(p.after,QPen(QColor("#1769aa"),0));
        outline(p.before,QPen(QColor("#b36b00"),0,Qt::DashLine));
        scene()->setSceneRect(scene()->itemsBoundingRect().adjusted(-1,-1,1,1));
    }
protected:
    void resizeEvent(QResizeEvent* event) override {
        QGraphicsView::resizeEvent(event);fitInView(scene()->sceneRect(),Qt::KeepAspectRatio);
    }
};
}
bool confirmJunctionPolygonFit(QWidget* parent,const JunctionPolygonPreview& p,const CatalogText& text) {
    QDialog dialog(parent);dialog.setObjectName("editorJunctionPolygonDialog");dialog.setWindowTitle(text("editorJunctionFitPolygon"));
    auto* body=scrollingBody(dialog);auto* layout=new QVBoxLayout(body);
    auto* note=new QLabel(text("editorJunctionPolygonNote").arg(QString::fromStdString(p.nodeId)),body);note->setWordWrap(true);layout->addWidget(note);
    layout->addWidget(new PolygonView(p,body));
    auto* issues=new QLabel(body);issues->setObjectName("editorJunctionPolygonIssues");issues->setWordWrap(true);QStringList reasons;
    for(const auto& issue:p.issues)reasons<<text(issue.code.c_str())+" — "+QString::fromStdString(issue.path);
    issues->setText(reasons.join('\n'));layout->addWidget(issues);
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&dialog);addFooter(dialog,buttons);
    buttons->button(QDialogButtonBox::Ok)->setText(text("editorJunctionFitPolygon"));buttons->button(QDialogButtonBox::Ok)->setEnabled(p.issues.empty());
    buttons->button(QDialogButtonBox::Cancel)->setText(text("editorCancel"));
    QObject::connect(buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);QObject::connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    dialog.resize(640,480);return dialog.exec()==QDialog::Accepted;
}
}
