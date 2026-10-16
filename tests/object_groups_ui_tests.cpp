// UI slice c (D195): the Objects dock shows one task group at a time. Every page is reached
// through its group in both languages, every existing jump reveals the owning group first, and
// navigation never touches the document, the selection or the run.
#include "../src/shell/editor_window.hpp"
#include "tab_page_helpers.hpp"
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLineEdit>
#include <QMenu>
#include <QTimer>
#include <QStandardPaths>
#include <QTableWidget>
#include <QTabWidget>
#include <QTest>
#include <iostream>
#include <set>
using namespace trafficsim;
namespace {
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
template<class T> T* item(QObject& root,const char* name){auto* value=root.findChild<T*>(name);require(value,name);return value;}
void settle(){QApplication::processEvents();QTest::qWait(20);}
void trigger(EditorWindow& window,const char* name){item<QAction>(window,name)->trigger();settle();}
QJsonObject catalog(const std::filesystem::path& data,int lang) {
    QFile file(QString::fromStdString((data/"locales"/(lang?"th.json":"en.json")).string()));
    require(file.open(QIODevice::ReadOnly),"Locale fixture missing");return QJsonDocument::fromJson(file.readAll()).object();
}
struct Page {const char* content;const char* label;const char* group;};
// SPEC §1's groups and in-group order; the content names the widget inside each page.
const Page pages[]={
    {"editorLinkTable","editorLinkTable","editorObjectGroupNetwork"},
    {"editorConnectorTable","editorConnectorTable","editorObjectGroupNetwork"},
    {"editorSignalTable","editorSignalTable","editorObjectGroupControl"},
    {"editorProgramTable","editorProgramTable","editorObjectGroupControl"},
    {"editorConflictTable","editorConflictTable","editorObjectGroupControl"},
    {"editorRouteTable","editorRouteTable","editorObjectGroupDemand"},
    {"editorInputTable","editorInputTable","editorObjectGroupDemand"},
    {"editorDecisionTable","editorDecisionTable","editorObjectGroupDemand"},
    {"editorZoneTable","editorZoneTable","editorObjectGroupDemand"},
    {"editorOdTable","editorOdTable","editorObjectGroupDemand"},
    {"editorCounterTable","editorCounterTable","editorObjectGroupMeasurement"},
    {"editorSectionTable","editorSectionTable","editorObjectGroupMeasurement"},
    {"editorResultsTabs","editorResultsTable","editorObjectGroupResults"},
    {"editorProblemTable","editorProblemTable","editorObjectGroupProblems"}};
const char* groups[]={"editorObjectGroupNetwork","editorObjectGroupControl","editorObjectGroupDemand",
    "editorObjectGroupMeasurement","editorObjectGroupResults","editorObjectGroupProblems"};

struct View {
    EditorWindow& window;QTabWidget* tabs;QComboBox* combo;
    QWidget* page(const char* content){return test::tabPage(tabs,item<QWidget>(window,content));}
    std::set<QWidget*> visible() const {
        std::set<QWidget*> out;for(int i=0;i<tabs->count();++i)if(tabs->isTabVisible(i))out.insert(tabs->widget(i));return out;
    }
    std::set<QWidget*> members(const QString& group) {
        std::set<QWidget*> out;for(const auto& p:pages)if(group==p.group)out.insert(page(p.content));return out;
    }
    QString group() const {return combo->currentData().toString();}
    // The group is shown exactly: its pages and no other, with the current page among them.
    void expect(const char* group,QWidget* current,const char* message) {
        require(this->group()==group&&visible()==members(group)&&tabs->currentWidget()==current,message);
    }
};
View view(EditorWindow& window) {
    View v{window,item<QTabWidget>(window,"editorObjectTabs"),item<QComboBox>(window,"editorObjectGroup")};
    require(v.tabs->count()==14&&v.combo->count()==6,"Page or group inventory changed");
    require(v.tabs->cornerWidget(Qt::TopLeftCorner)==v.combo,"The group selector is not on the tab-strip row");
    for(int i=0;i<6;++i)require(v.combo->itemData(i).toString()==groups[i],"Group order differs from the specification");
    return v;
}

void everyPage(EditorWindow& window,const std::filesystem::path& data) {
    auto v=view(window);auto* language=item<QComboBox>(window,"editorLanguage");
    v.expect("editorObjectGroupNetwork",v.page("editorLinkTable"),"Default group is not Network/Links");
    // Forcing probe: a direct jump to a page of a hidden group must land, then reveal that group.
    auto* zones=v.page("editorZoneTable");
    require(!v.tabs->isTabVisible(v.tabs->indexOf(zones)),"Zones fixture was not hidden");
    v.tabs->setCurrentWidget(zones);settle();
    require(v.tabs->currentWidget()==zones,"A jump to a hidden-group page did not take effect");
    v.expect("editorObjectGroupDemand",zones,"A jump did not reveal its owning group");

    auto* canvas=window.canvas();canvas->select(window.history().document().network.links.front().id);settle();
    const auto revision=window.history().revision();const auto selection=canvas->selected();const auto tick=window.runState().tick;
    require(!selection.empty(),"Selection baseline missing");
    for(int lang:{0,1}) {
        language->setCurrentIndex(lang);settle();
        const auto words=catalog(data,lang),other=catalog(data,1-lang);
        require(v.combo->accessibleName()==words.value("editorObjectGroup").toString(),"Selector has no translated name");
        for(int g=0;g<6;++g) {
            v.combo->setCurrentIndex(g);settle();
            require(v.combo->itemText(g)==words.value(groups[g]).toString(),"Group name untranslated");
            QWidget* first=nullptr;for(const auto& p:pages)if(!first&&QString(groups[g])==p.group)first=v.page(p.content);
            v.expect(groups[g],first,"Choosing a group did not show exactly its pages");
            for(const auto& p:pages) {
                if(QString(groups[g])!=p.group)continue;
                auto* page=v.page(p.content);v.tabs->setCurrentWidget(page);settle();
                v.expect(p.group,page,"A page of the shown group did not open");
                require(v.tabs->tabText(v.tabs->indexOf(page))==words.value(p.label).toString(),"Page label untranslated");
                language->setCurrentIndex(1-lang);settle();
                v.expect(p.group,page,"A locale switch moved the page or group");
                require(v.tabs->tabText(v.tabs->indexOf(page))==other.value(p.label).toString()&&
                        v.combo->currentText()==other.value(p.group).toString(),"A locale switch left stale labels");
                language->setCurrentIndex(lang);settle();
            }
        }
    }
    require(window.history().revision()==revision&&canvas->selected()==selection&&window.runState().tick==tick,
            "Group navigation changed the document, selection or run");
    language->setCurrentIndex(0);settle();
}

void jumps(EditorWindow& window) {
    auto v=view(window);
    for(const auto& target:{std::tuple{9,"editorConflictTable","editorObjectGroupControl"},
                            std::tuple{10,"editorCounterTable","editorObjectGroupMeasurement"},
                            std::tuple{11,"editorSectionTable","editorObjectGroupMeasurement"},
                            std::tuple{12,"editorZoneTable","editorObjectGroupDemand"}}) {
        v.combo->setCurrentIndex(0);settle();
        item<QComboBox>(window,"editorTool")->setCurrentIndex(std::get<0>(target));settle();
        v.expect(std::get<2>(target),v.page(std::get<1>(target)),"A tool did not reveal its table's group");
    }
    item<QComboBox>(window,"editorTool")->setCurrentIndex(0);
    // Show in table, through the canvas context menu on a vehicle input.
    auto* canvas=window.canvas();canvas->fitNetwork();settle();
    QPoint at;bool found=false;std::string kind;
    for(int y=0;y<canvas->viewport()->height()&&!found;y+=3)for(int x=0;x<canvas->viewport()->width()&&!found;x+=3)
        if(canvas->demandObjectAt({x,y}).first=="input"){at={x,y};found=true;}
    require(found,"Input fixture has no vehicle input on the canvas");
    const auto input=canvas->demandObjectAt(at).second;
    v.combo->setCurrentIndex(0);settle();bool revealed=false;
    QTimer::singleShot(150,[&]{
        auto* menu=qobject_cast<QMenu*>(QApplication::activePopupWidget());if(!menu)return;
        auto* reveal=menu->findChild<QAction*>("editorDemandMenuReveal");
        if(!reveal){menu->close();return;}
        revealed=true;menu->setActiveAction(reveal);QTest::keyClick(menu,Qt::Key_Return);
    });
    canvas->contextMenuRequested(at);settle();
    require(revealed,"Show in table was not offered");
    auto* inputs=item<QTableWidget>(window,"editorInputTable");
    v.expect("editorObjectGroupDemand",v.page("editorInputTable"),"Show in table did not reveal Demand/Vehicle inputs");
    require(!inputs->selectedItems().empty()&&inputs->selectedItems().front()->data(Qt::UserRole).toString().toStdString()==input,
            "Show in table did not select the input");
}

void problemsAndResults(EditorWindow& window,const std::filesystem::path& data) {
    auto v=view(window);auto* problems=v.page("editorProblemTable");
    trigger(window,"editorRecheck");
    v.expect("editorObjectGroupProblems",problems,"Recheck did not reveal Problems");
    bool fails=false;
    try{compileDocument(window.history().document(),data);}catch(const std::exception&){fails=true;}
    require(fails,"Empty-network batch failure fixture unexpectedly compiled");
    auto* seeds=item<QLineEdit>(window,"editorSeeds");seeds->setText("42-43");
    v.combo->setCurrentIndex(0);settle();trigger(window,"editorRunSeeds");
    v.expect("editorObjectGroupProblems",problems,"A failed batch did not reveal Problems");
    window.openFile(QString::fromStdString((data/"scenarios/crossing.json").string()));settle();
    auto* seed=item<QLineEdit>(window,"editorSeed");seed->setText("invalid");
    require(seed->text()=="invalid","Invalid-seed failure fixture was not set");
    v.combo->setCurrentIndex(0);settle();trigger(window,"editorStep");
    v.expect("editorObjectGroupProblems",problems,"A failed Run did not reveal Problems");
    seed->setText("42");
    auto* inner=item<QTabWidget>(window,"editorResultsTabs");auto* results=v.page("editorResultsTabs");
    auto* clamps=test::tabPage(inner,item<QWidget>(window,"editorClampTable"));
    inner->setCurrentWidget(clamps);v.combo->setCurrentIndex(0);settle();
    trigger(window,"editorRunSeeds");
    auto* movements=test::tabPage(inner,item<QWidget>(window,"editorMovementTable"));
    v.expect("editorObjectGroupResults",results,"A batch did not reveal Results");
    require(inner->count()==4&&inner->currentWidget()==movements,"A batch did not replace Results page 0 only");
    trigger(window,"editorCancelSeeds");
    v.expect("editorObjectGroupResults",results,"Cancel left Results");
    require(inner->count()==4&&inner->currentWidget()==movements,"Cancel moved Results off page 0");
    seeds->setText("42-51");
}

void layout(EditorWindow& window) {
    auto v=view(window);auto* language=item<QComboBox>(window,"editorLanguage");auto* canvas=window.canvas();
    for(const auto& size:{QSize(1360,860),QSize(1024,768)})for(int lang:{0,1}) {
        language->setCurrentIndex(lang);window.resize(size);settle();
        const QSize minimum=size.width()>1100?QSize(700,350):QSize(400,250);
        for(int g=0;g<6;++g) {
            v.combo->setCurrentIndex(g);settle();
            require(canvas->viewport()->width()>=minimum.width()&&canvas->viewport()->height()>=minimum.height(),
                    "A group crowded out the canvas");
            require(v.combo->isVisible()&&window.rect().contains(QRect(v.combo->mapTo(&window,QPoint()),v.combo->size())),
                    "Group selector outside the window");
        }
    }
    v.combo->setCurrentIndex(3);settle();auto* counters=v.page("editorCounterTable");v.tabs->setCurrentWidget(counters);settle();
    trigger(window,"editorFocusCanvas");trigger(window,"editorFocusCanvas");
    v.expect("editorObjectGroupMeasurement",counters,"Focus mode lost the current group");
    trigger(window,"editorResetLayout");
    v.expect("editorObjectGroupNetwork",v.page("editorLinkTable"),"Reset did not return Network/Links");
}
}
int main(int argc,char** argv) {
    QApplication app(argc,argv);QStandardPaths::setTestModeEnabled(true);
    try {
        require(argc>=2,"Expected data directory");const std::filesystem::path data(argv[1]);
        {
            EditorWindow window(data);window.resize(1360,860);window.show();settle();
            window.openFile(QString::fromStdString((data/"projects/four-leg-signalised.traffic.json").string()));settle();
            everyPage(window,data);jumps(window);layout(window);
        }
        {EditorWindow window(data);window.resize(1360,860);window.show();settle();problemsAndResults(window,data);}
        std::cout<<"Every Objects page is reached through its group; jumps reveal the owning group\n";return 0;
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
