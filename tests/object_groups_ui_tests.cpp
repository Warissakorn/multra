#include "../src/shell/editor_window.hpp"
#include "tab_page_helpers.hpp"
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDockWidget>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QStandardPaths>
#include <QTabBar>
#include <QTabWidget>
#include <QTest>
#include <filesystem>
#include <iostream>
#include <set>
#include <vector>
using namespace trafficsim;
// UI slice c (B16, layout A): the Objects dock shows one task group of pages at a time.
// Every page stays one registered widget; the group follows whichever page is current.
namespace {
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
template<class T> T* item(QObject& root,const char* name){auto* value=root.findChild<T*>(name);require(value,"Missing object-groups widget");return value;}
void settle(){QApplication::processEvents();QTest::qWait(20);}
void trigger(EditorWindow& window,const char* name){item<QAction>(window,name)->trigger();settle();}
QJsonObject catalog(const std::filesystem::path& data,int lang) {
    QFile file(QString::fromStdString((data/"locales"/(lang?"th.json":"en.json")).string()));
    require(file.open(QIODevice::ReadOnly),"Locale fixture missing");return QJsonDocument::fromJson(file.readAll()).object();
}
struct Group {const char* key;std::vector<const char*> pages;};
// Page contents by object name, in the spec's order (UI_PRESENTATION_SPEC §1).
const Group groups[]={
    {"editorObjectGroupNetwork",{"editorLinkTable","editorConnectorTable"}},
    {"editorObjectGroupControl",{"editorSignalTable","editorProgramTable","editorConflictTable"}},
    {"editorObjectGroupDemand",{"editorRouteTable","editorInputTable","editorDecisionTable","editorSpeedDecisionTable","editorReducedSpeedAreaTable","editorZoneTable","editorOdTable","editorPeriodTable"}},
    {"editorObjectGroupMeasurement",{"editorCounterTable","editorSectionTable","editorNodeTable"}},
    {"editorObjectGroupResults",{"editorResultsTabs"}},
    {"editorObjectGroupProblems",{"editorProblemTable"}}};
QString groupKey(QComboBox* combo){return combo->currentData().toString();}
// The pages a user can reach in the strip right now, in visual order.
std::vector<QWidget*> visiblePages(QTabWidget* tabs) {
    std::vector<QWidget*> shown;
    for(int i=0;i<tabs->count();++i)if(tabs->isTabVisible(i))shown.push_back(tabs->widget(i));
    return shown;
}
void pick(QComboBox* combo,const char* key) {
    const int index=combo->findData(QString::fromLatin1(key));require(index>=0,"Group missing from selector");
    combo->setCurrentIndex(index);settle();
}
}
int main(int argc,char** argv) {
    QApplication app(argc,argv);QStandardPaths::setTestModeEnabled(true);
    try {
        require(argc>=2,"Expected data directory");const std::filesystem::path data(argv[1]);
        EditorWindow window(data);window.resize(1360,860);window.show();settle();
        window.openFile(QString::fromStdString((data/"scenarios/crossing.json").string()));settle();
        auto* tabs=item<QTabWidget>(window,"editorObjectTabs");auto* combo=item<QComboBox>(window,"editorObjectGroup");
        auto* language=item<QComboBox>(window,"editorLanguage");auto* canvas=window.canvas();
        auto* dock=item<QDockWidget>(window,"editorObjectsDock");
        require(tabs->count()==18,"Object page inventory changed");
        require(tabs->cornerWidget(Qt::TopLeftCorner)==combo,"Group selector is not in the page strip row");
        require(combo->count()==6,"Selector does not list six task groups");
        // Default after construction and Reset: Network, Links.
        require(groupKey(combo)=="editorObjectGroupNetwork"&&tabs->currentWidget()==item<QWidget>(window,"editorLinkTable"),"Default group is not Network/Links");
        const auto revision=window.history().revision();const auto tick=window.runState().tick;
        std::set<QWidget*> covered;
        for(int lang:{0,1}) {
            language->setCurrentIndex(lang);settle();const auto text=catalog(data,lang);
            require(!text.value("editorObjectGroup").toString().isEmpty()&&combo->accessibleName()==text.value("editorObjectGroup").toString(),"Selector has no accessible name");
            for(int g=0;g<6;++g) {
                const auto& group=groups[g];
                require(combo->itemData(g).toString()==group.key,"Group order changed");
                require(!text.value(group.key).toString().isEmpty()&&combo->itemText(g)==text.value(group.key).toString(),"Group label untranslated");
                pick(combo,group.key);
                std::vector<QWidget*> expected;
                for(const auto* content:group.pages)expected.push_back(test::tabPage(tabs,item<QWidget>(window,content)));
                require(visiblePages(tabs)==expected,"Group shows the wrong pages");
                require(tabs->currentWidget()==expected.front(),"Choosing a group did not open its first page");
                for(auto* page:expected)covered.insert(page);
            }
        }
        require(covered.size()==18,"Some page belongs to no group");
        // Any jump to a hidden page, in either language, reveals that page's group, and a locale
        // switch while it is current keeps both.
        for(int lang:{0,1})for(const auto& owner:groups)for(const auto* content:owner.pages) {
            language->setCurrentIndex(lang);settle();
            auto* page=test::tabPage(tabs,item<QWidget>(window,content));
            pick(combo,owner.key==std::string("editorObjectGroupNetwork")?"editorObjectGroupProblems":"editorObjectGroupNetwork");
            require(!tabs->isTabVisible(tabs->indexOf(page)),"Jump fixture page was not hidden");
            tabs->setCurrentWidget(page);settle();
            require(tabs->currentWidget()==page&&tabs->isTabVisible(tabs->indexOf(page)),"Jump left its page hidden");
            require(groupKey(combo)==owner.key,"Jump did not reveal the owning group");
            language->setCurrentIndex(1-lang);settle();
            require(tabs->currentWidget()==page&&groupKey(combo)==owner.key,"Locale switch lost the page or group");
        }
        language->setCurrentIndex(0);settle();
        // Tool navigation (show helpers) reveals the group.
        for(const auto& [tool,key]:{std::pair{9,"editorObjectGroupControl"},std::pair{10,"editorObjectGroupMeasurement"},
                                     std::pair{11,"editorObjectGroupMeasurement"},std::pair{12,"editorObjectGroupDemand"},std::pair{13,"editorObjectGroupMeasurement"}}) {
            pick(combo,"editorObjectGroupResults");
            item<QComboBox>(window,"editorTool")->setCurrentIndex(tool);settle();
            require(groupKey(combo)==key,"Tool navigation did not reveal its group");
        }
        item<QComboBox>(window,"editorTool")->setCurrentIndex(0);settle();
        // A failed Run reveals Problems from any group, and the dock is shown first.
        auto* seed=item<QLineEdit>(window,"editorSeed");seed->setText("invalid");
        require(seed->text()=="invalid","Invalid-seed fixture was not set");
        pick(combo,"editorObjectGroupDemand");dock->hide();settle();require(!dock->isVisible(),"Dock hide fixture failed");
        trigger(window,"editorStep");
        require(groupKey(combo)=="editorObjectGroupProblems"&&tabs->currentWidget()==item<QWidget>(window,"editorProblemTable"),"Failed Run did not reveal Problems");
        require(dock->isVisible(),"Failed Run left the Problems dock hidden");
        seed->setText("42");
        // Focus keeps the group; Reset returns Network/Links. Neither edits the document or run.
        pick(combo,"editorObjectGroupMeasurement");
        trigger(window,"editorFocusCanvas");require(!dock->isVisible(),"Focus fixture did not hide the dock");
        trigger(window,"editorFocusCanvas");
        require(dock->isVisible()&&groupKey(combo)=="editorObjectGroupMeasurement","Focus forgot the current group");
        trigger(window,"editorResetLayout");
        require(groupKey(combo)=="editorObjectGroupNetwork"&&tabs->currentWidget()==item<QWidget>(window,"editorLinkTable"),"Reset did not return Network/Links");
        require(window.history().revision()==revision&&window.runState().tick==tick,"Group navigation changed the document or run");
        // The selector shares the strip row: the dock is no taller than its tab bar plus a page.
        require(combo->height()<=tabs->tabBar()->height()+8,"Selector is taller than the page strip");
        // Every group leaves a usable canvas at both sizes and in both languages.
        for(const auto& size:{QSize(1360,860),QSize(1024,768)})for(int lang:{0,1}) {
            language->setCurrentIndex(lang);window.resize(size);settle();
            for(const auto& group:groups) {
                pick(combo,group.key);
                require(window.width()<=size.width()&&window.height()<=size.height(),"A group forced the window larger");
                const auto view=canvas->viewport()->size();
                if(size.width()==1024)require(view.width()>=400&&view.height()>=250,"Small-window canvas is unusable with a group");
                else require(view.width()>=700&&view.height()>=350,"Wide canvas crowded out by a group");
                require(window.rect().contains(QRect(combo->mapTo(&window,QPoint()),combo->size())),"Selector outside window");
                require(combo->mapTo(tabs,QPoint(combo->width(),0)).x()<=tabs->tabBar()->mapTo(tabs,QPoint()).x(),"Selector overlaps the page strip");
            }
        }
        if(argc>2) {
            language->setCurrentIndex(0);window.resize(1024,768);pick(combo,"editorObjectGroupDemand");
            require(window.grab().save(QString::fromUtf8(argv[2])+"-en.png"),"English screenshot failed");
            language->setCurrentIndex(1);settle();
            require(window.grab().save(QString::fromUtf8(argv[2])+"-th.png"),"Thai screenshot failed");
        }
        std::cout<<"Object groups: inventory, jumps, locales, focus, reset and sizing passed\n";return 0;
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
