#include "../src/shell/editor_window.hpp"
#include "../src/shell/object_export.hpp"
#include "ui_test_storage.hpp"
#include <nlohmann/json.hpp>
#include <QApplication>
#include <QAction>
#include <QComboBox>
#include <QFile>
#include <QFileInfo>
#include <QFileDialog>
#include <QDir>
#include <QMenu>
#include <QLineEdit>
#include <QTemporaryDir>
#include <QTimer>
#include <iostream>
using namespace multra;
namespace {
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
QByteArray bytes(const QString& path){QFile f(path);require(f.open(QIODevice::ReadOnly),"Cannot read export");return f.readAll();}
}
int main(int argc,char** argv) {
    QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
    QApplication app(argc,argv);useTestStorage();
    try {
        require(argc==2,"Expected data path");const std::filesystem::path data(argv[1]);
        QTemporaryDir dir;require(dir.isValid(),"Temp directory failed");EditorWindow window(data,"en");
        window.openFile(QString::fromStdString((data/"projects/four-leg-signalised.traffic.json").string()));
        window.canvas()->select(window.history().document().network.links.front().id);
        auto* name=window.findChild<QLineEdit*>("editorName");require(name && name->isEnabled(),"Name editor unavailable");
        name->setText(QString::fromUtf8("ถนน, ฉบับแก้ไข"));QMetaObject::invokeMethod(name,"editingFinished");
        require(window.history().dirty() && window.history().document().network.links.front().name=="ถนน, ฉบับแก้ไข","Unsaved edit fixture failed");
        const auto before=documentJson(window.history().document());
        const auto dirty=window.history().dirty();const auto revision=window.history().document().revision;
        auto* menu=window.findChild<QMenu*>("editorExportObjects");require(menu && menu->actions().size()==5,"Missing object export menu");
        for(const auto& [name,table]:{std::pair{"editorExportLinks",ObjectTable::links},{"editorExportConnectors",ObjectTable::connectors},
            {"editorExportInputs",ObjectTable::inputs},{"editorExportRoutes",ObjectTable::routes},{"editorExportDecisions",ObjectTable::decisions}}) {
            auto* action=window.findChild<QAction*>(name);require(action && action->isEnabled(),"Export needs no Run");
            const auto path=dir.path()+"/ตาราง-"+objectTableName(table)+".csv";bool selected=false;
            QTimer::singleShot(30,[&]{
                auto* dialog=window.findChild<QFileDialog*>("editorObjectCsvDialog");
                if(dialog){
                    if(auto* edit=dialog->findChild<QLineEdit*>("fileNameEdit"))edit->setText(path);
                    selected=dialog->selectedFiles()==QStringList{path};
                    QMetaObject::invokeMethod(dialog,selected?"accept":"reject");
                }
            });
            action->trigger();require(selected,"Save dialog was not opened");
            require(bytes(path)==QByteArray::fromStdString(objectTableCsv(window.history().document(),table)),"UI exported wrong table");
        }
        const auto filesBeforeCancel=QDir(dir.path()).entryList(QDir::Files);bool cancelled=false;
        QTimer::singleShot(30,[&]{if(auto* d=window.findChild<QFileDialog*>("editorObjectCsvDialog")){cancelled=true;d->reject();}});
        window.findChild<QAction*>("editorExportLinks")->trigger();require(cancelled,"Cancel fixture did not open dialog");
        require(QDir(dir.path()).entryList(QDir::Files)==filesBeforeCancel,"Cancel wrote a file");
        const auto sentinel=dir.path()+"/project.traffic.json";
        {QFile f(sentinel);require(f.open(QIODevice::WriteOnly),"Sentinel creation failed");require(f.write("keep")==4,"Sentinel write failed");}
        require(bytes(sentinel)=="keep","Sentinel fixture failed");bool refused=false;
        try{writeObjectTableCsv(sentinel,window.history().document(),ObjectTable::links,{sentinel});}catch(const std::exception&){refused=true;}
        require(refused && bytes(sentinel)=="keep","Protected project was replaced");
        require(QFileInfo(dir.path()).isDir(),"Failure fixture is not a directory");refused=false;
        try{writeObjectTableCsv(dir.path(),window.history().document(),ObjectTable::links);}catch(const std::exception&){refused=true;}
        require(refused && QFileInfo(dir.path()).isDir(),"Failed write damaged destination");
        window.findChild<QComboBox*>("editorLanguage")->setCurrentIndex(1);
        require(menu->title()==QString::fromUtf8("ส่งออกตารางวัตถุ (CSV)"),"Thai menu missing");
        require(documentJson(window.history().document())==before,"Export mutated document");
        require(window.history().dirty()==dirty && window.history().document().revision==revision,"Export changed save/revision state");
        std::cout<<"Object CSV UI exports, cancellation, protection and translation passed\n";return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
