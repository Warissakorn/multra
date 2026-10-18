#include "../src/shell/editor_window.hpp"
#include "../src/shell/editor_interaction.hpp"
#include "../src/editor/ui_design_tokens.hpp"
#include <QAbstractScrollArea>
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDockWidget>
#include <QFormLayout>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QScrollArea>
#include <QScrollBar>
#include <QStandardPaths>
#include <QTabWidget>
#include <QTest>
#include <QToolButton>
#include <filesystem>
#include <iostream>
#include <vector>
using namespace trafficsim;
// UI slice d1: the Properties dock's forms share one label column, wrap at narrow width, and keep
// their commit boundaries. Presentation only: nothing here may change when an edit commits.
namespace {
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
template<class T> T* item(QObject& root,const char* name){auto* value=root.findChild<T*>(name);require(value,"Missing Properties widget");return value;}
void settle(){QApplication::processEvents();QTest::qWait(20);}
struct Pair {QWidget* label;QWidget* field;};
std::vector<Pair> pairs(QFormLayout* form) {
    std::vector<Pair> out;
    for(int row=0;row<form->rowCount();++row) {
        auto* l=form->itemAt(row,QFormLayout::LabelRole);auto* f=form->itemAt(row,QFormLayout::FieldRole);
        if(!l||!f||!l->widget()||!f->widget())continue;
        if(!dynamic_cast<ReflowLabel*>(l->widget())||!l->widget()->isVisibleTo(form->parentWidget()))continue;
        out.push_back({l->widget(),f->widget()});
    }
    return out;
}
// The top group (ID/Name, appearance, behaviour) and the three tab pages each share a column.
std::vector<QFormLayout*> forms(QWidget* body,bool pages) {
    std::vector<QFormLayout*> out;auto* tabs=item<QTabWidget>(*body,"editorPropertyTabs");
    for(auto* form:body->findChildren<QFormLayout*>())
        if(tabs->isAncestorOf(form->parentWidget())==pages&&!pairs(form).empty())out.push_back(form);
    return out;
}
void showAllPages(EditorWindow& window) {
    auto* toggle=item<QAction>(window,"editorAppearance");if(!toggle->isChecked())toggle->trigger();settle();
}
// Every label/field pair laid out on one row starts its field at one x within its group.
void aligned(EditorWindow& window,const char* where,bool beside=true) {
    auto* body=item<QDockWidget>(window,"editorInspectorDock")->widget();auto* tabs=item<QTabWidget>(window,"editorPropertyTabs");
    for(bool pages:{false,true}) {
        int x=-1,rows=0;
        for(int t=0;t<(pages?tabs->count():1);++t) {
            if(pages){tabs->setCurrentIndex(t);settle();}
            for(auto* form:forms(body,pages)) {
                if(pages&&!tabs->currentWidget()->isAncestorOf(form->parentWidget()))continue;
                for(const auto& p:pairs(form)) {
                    require(p.label->height()>=p.label->heightForWidth(p.label->width()),"A Properties label is clipped");
                    const auto label=QRect(p.label->mapTo(body,QPoint()),p.label->size());
                    const auto field=QRect(p.field->mapTo(body,QPoint()),p.field->size());
                    if(field.top()>=label.bottom())continue; // wrapped below its label
                    ++rows;
                    if(!beside) { // D242: Windows is the only host where this has failed, so name what stayed beside
                        auto* host=form->parentWidget();auto* scroll=qobject_cast<QAbstractScrollArea*>(host->parentWidget()?host->parentWidget()->parentWidget():nullptr);
                        std::cerr<<where<<": "<<p.field->objectName().toStdString()<<" beside its label; label x "<<label.left()<<" w "<<label.width()
                                 <<", field x "<<field.left()<<" w "<<field.width()<<", host "<<host->width()<<", policy "<<form->rowWrapPolicy()
                                 <<", viewport "<<(scroll?scroll->viewport()->width():-1)<<'\n';
                    }
                    require(field.width()>=80,"A field beside its label is squeezed");
                    if(x<0)x=field.left();
                    else if(std::abs(field.left()-x)>1){std::cerr<<where<<": "<<p.field->objectName().toStdString()<<" at "<<field.left()<<", column at "<<x<<'\n';require(false,"Properties fields do not share one column");}
                }
            }
        }
        require(beside?rows>0:rows==0,beside?"No Properties row sits beside its label":"A narrow form kept a field beside its label");
    }
    tabs->setCurrentIndex(0);settle();
}
int rowOf(QFormLayout* form,QWidget* field){int row=-1;QFormLayout::ItemRole role;form->getWidgetPosition(field,&row,&role);return row;}
}
int main(int argc,char** argv) {
    QApplication app(argc,argv);QStandardPaths::setTestModeEnabled(true);
    try {
        require(argc>=2,"Expected data directory");const std::filesystem::path data(argv[1]);
        EditorWindow window(data);window.resize(1360,860);window.show();settle();
        window.openFile(QString::fromStdString((data/"scenarios/crossing.json").string()));settle();
        auto* canvas=window.canvas();auto* language=item<QComboBox>(window,"editorLanguage");
        auto* dock=item<QDockWidget>(window,"editorInspectorDock");
        canvas->select(window.history().document().network.links.front().id);settle();
        showAllPages(window);
        if(argc>2) { // before/after evidence, taken before any assertion
            const QString prefix=QString::fromUtf8(argv[2]);
            require(dock->grab().save(prefix+"-en-1360.png"),"Screenshot failed");
            language->setCurrentIndex(1);window.resize(1024,768);settle();
            require(dock->grab().save(prefix+"-th-1024.png"),"Screenshot failed");
            dock->setFloating(true);dock->resize(200,700);settle();
            require(dock->grab().save(prefix+"-th-narrow.png"),"Screenshot failed");
            dock->setFloating(false);language->setCurrentIndex(0);window.resize(1360,860);settle();
        }
        // The long markings label wraps in the column instead of pushing every Link field down.
        auto* markings=item<QLineEdit>(window,"editorLinkMarkings");
        auto* linkForm=qobject_cast<QFormLayout*>(markings->parentWidget()->layout());require(linkForm,"Link form missing");
        require(markings->y()<linkForm->labelForField(markings)->geometry().bottom(),"Markings field wrapped below its label at 1360");
        // At 1024×768 the open appearance fold does not fit above the pages (it did not before D195
        // either; OWNER_SITTING E17), so the small window is checked in its default, folded state.
        auto* fold=item<QAction>(window,"editorAppearance");
        for(const auto& size:{QSize(1360,860),QSize(1024,768)})for(int lang:{0,1}) {
            if(fold->isChecked()==(size.width()==1024))fold->trigger();
            language->setCurrentIndex(lang);window.resize(size);settle();
            aligned(window,lang?"th":"en");
        }
        // D205: a label with no break point (Thai on Windows, where Qt has no libthai, measured 123 px)
        // cannot wrap. A little wider than the column, its row stays beside it, and the column it widens
        // is every form's, not only its own. The forcing first: the word is just wider than the column.
        if(!fold->isChecked())fold->trigger();
        language->setCurrentIndex(0);window.resize(1360,860);settle();
        auto* gapLabel=qobject_cast<QLabel*>(qobject_cast<QFormLayout*>(item<QWidget>(window,"editorMedianGap")->parentWidget()->layout())
                                             ->labelForField(item<QWidget>(window,"editorMedianGap")));
        require(gapLabel,"Median gap label missing");
        // Windows' word was 3 px over; a narrow letter reaches that without overshooting into a wrap.
        for(int n=1;gapLabel->QLabel::minimumSizeHint().width()<editorDesign::formLabelWidth+3;++n){gapLabel->setText(QString(n,QChar('i')));settle();}
        const int word=gapLabel->QLabel::minimumSizeHint().width();
        require(word>editorDesign::formLabelWidth+1&&word<editorDesign::formLabelWidth*5/4,"Setup: the unbreakable label is not just wider than the column");
        require(gapLabel->width()>=word,"Setup: the unbreakable label's row wrapped instead of widening");
        aligned(window,"unbreakable");
        // The column narrows back while the appearance fold is closed (Windows: Thai at 1360, then English
        // at 1024): a hidden form must not keep the old column. The forcing first: the fold is hidden.
        fold->trigger();settle();
        require(!item<QComboBox>(window,"editorDrivingSide")->isVisible(),"Setup: the appearance fold is still shown");
        language->setCurrentIndex(1);settle();language->setCurrentIndex(0);settle(); // the table restores the text
        require(gapLabel->QLabel::minimumSizeHint().width()<=editorDesign::formLabelWidth,"Setup: the label text was not restored");
        aligned(window,"hidden fold");
        fold->trigger();settle();
        aligned(window,"restored");
        // Narrow floating panel: fields drop below their labels, nothing clips or scrolls sideways.
        dock->setFloating(true);dock->resize(200,700);settle();
        require(dock->width()<=220,"Narrow fixture did not take effect");
        for(int lang:{0,1}) {
            language->setCurrentIndex(lang);settle();aligned(window,"narrow",false);
            auto* tabs=item<QTabWidget>(window,"editorPropertyTabs");
            for(int t=0;t<tabs->count();++t) {
                tabs->setCurrentIndex(t);settle();
                // Labels and fields fit; only text-only action buttons, which cannot wrap, may need the
                // page's horizontal scroll (they did before D195 too; OWNER_SITTING E17).
                auto* page=qobject_cast<QScrollArea*>(tabs->currentWidget());
                for(auto* w:page->widget()->findChildren<QWidget*>())
                    if(w->isVisibleTo(page->widget())&&!qobject_cast<QToolButton*>(w)&&w->parentWidget()==page->widget())
                        require(w->minimumSizeHint().width()<=page->viewport()->width(),"A narrow Properties label or field is wider than its page");
            }
            tabs->setCurrentIndex(0);settle();
        }
        // D242: on Windows a page's widest action button made the Link page wide enough for two columns
        // (Linux: 248 px, 8 px short of it), so its form kept rows beside their labels in a 188 px viewport.
        // The forcing first: a longer button text makes the page that wide.
        {
            language->setCurrentIndex(0);settle();
            auto* addPoint=item<QAction>(window,"editorAddLinkPoint");
            auto* page=qobject_cast<QScrollArea*>(item<QTabWidget>(window,"editorPropertyTabs")->widget(0));
            const auto margins=page->widget()->layout()->contentsMargins();
            const int twoColumns=2*editorDesign::formLabelWidth+margins.left()+margins.right();
            for(int n=1;page->widget()->minimumSizeHint().width()<twoColumns+40;++n){addPoint->setText(QString(n,QChar('M')));settle();}
            // Only the viewport narrows: from about 290 px, two columns' worth, to 188, with the page at its minimum.
            dock->resize(300,700);settle();const int before=page->widget()->width();
            require(page->viewport()->width()>=twoColumns&&before>page->viewport()->width(),"Setup: the wide viewport does not show two columns");
            require(linkForm->rowWrapPolicy()==QFormLayout::WrapLongRows,"A viewport wide enough for two columns kept the Link form stacked");
            dock->resize(200,700);settle();
            require(page->widget()->width()==before&&page->widget()->width()>page->viewport()->width(),"Setup: the Link page is not wider than its viewport");
            aligned(window,"wide page",false);
            language->setCurrentIndex(1);settle();language->setCurrentIndex(0);settle(); // the table restores the text
            require(page->widget()->minimumSizeHint().width()<twoColumns,"Setup: the button text was not restored");
        }
        // D243, the Windows run: the appearance toggle's text, which cannot wrap, is wider there, so the floating
        // panel could not narrow below about 289 px and its top forms kept two columns (the D242 diagnostic:
        // host 273 px, no viewport). A long text now elides instead. The forcing first: the text alone is that wide.
        {
            language->setCurrentIndex(0);settle();
            auto* toggle=item<QToolButton>(window,"editorAppearanceToggle");auto* appearance=item<QAction>(window,"editorAppearance");
            for(int n=1;toggle->sizeHint().width()<2*editorDesign::formLabelWidth+40;++n){appearance->setText(QString(n,QChar('M')));settle();}
            dock->resize(200,700);settle();
            require(dock->width()<=220,"A long top-group button widened the narrow panel");
            require(toggle->width()<toggle->sizeHint().width(),"The long toggle text did not shrink to the panel");
            aligned(window,"long toggle",false);
            language->setCurrentIndex(1);settle();language->setCurrentIndex(0);settle(); // the table restores the text
            require(toggle->sizeHint().width()<2*editorDesign::formLabelWidth,"Setup: the toggle text was not restored");
        }
        dock->setFloating(false);language->setCurrentIndex(0);window.resize(1360,860);settle();
        // Immediate settings and Apply sections stay apart, divided, and keep their commit triggers.
        auto* side=item<QComboBox>(window,"editorDrivingSide");auto* level=item<QComboBox>(window,"editorObjectLevel");
        auto* divider=item<QFrame>(window,"editorApplyDivider");
        require(divider->frameShape()==QFrame::HLine&&divider->isVisible(),"Apply divider missing");
        require(side->y()<divider->y()&&divider->y()<level->y()&&level->y()-divider->y()>=8,"Divider does not separate driving side from Apply");
        canvas->select(window.history().document().network.links.front().id);settle();
        auto revision=window.history().revision();
        side->setCurrentIndex(1-side->currentIndex());settle();
        require(window.history().revision()==revision+1,"Driving side no longer commits immediately");
        revision=window.history().revision();
        require(level->count()>1,"Level fixture has one choice");
        level->setCurrentIndex((level->currentIndex()+1)%level->count());settle();
        require(window.history().revision()==revision,"Level committed without Apply");
        item<QAction>(window,"editorApplyDisplay")->trigger();settle();
        require(window.history().revision()==revision+1,"Apply did not commit the level");
        revision=window.history().revision();
        auto* name=item<QLineEdit>(window,"editorName");name->setFocus();name->selectAll();QTest::keyClicks(name,"renamed");settle();
        require(window.history().revision()==revision,"Name committed before editing finished");
        QTest::keyClick(name,Qt::Key_Return);settle();
        require(window.history().revision()==revision+1,"Name did not commit on editing finished");
        // Headings, translated in both languages; connector positions now follow the lane counts.
        for(int lang:{0,1}) {
            language->setCurrentIndex(lang);settle();
            for(const auto* key:{"editorGeometryHeading","editorSplitHeading","editorAttachmentsHeading","editorShapeHeading","editorPlacementHeading"}) {
                auto* heading=item<QLabel>(window,key);
                require(!heading->text().isEmpty()&&heading->property("editorEyebrow").toBool(),"Properties heading untranslated or unstyled");
            }
        }
        auto* from=item<QWidget>(window,"editorFromPosition");auto* connectorForm=qobject_cast<QFormLayout*>(from->parentWidget()->layout());
        require(connectorForm&&rowOf(connectorForm,from)<rowOf(connectorForm,item<QWidget>(window,"editorConnectorPoints"))&&
                rowOf(connectorForm,item<QWidget>(window,"editorToLaneCount"))<rowOf(connectorForm,from),"Connector positions are not with the attachments");
        std::cout<<"Properties: shared column, narrow reflow, divider, commit boundaries and headings passed\n";return 0;
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
