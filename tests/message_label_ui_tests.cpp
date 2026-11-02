// D429: the message line under the canvas shows one row -- the first line elided, then
// "and N more" -- keeps the whole message as its text and tooltip, and closes until a different
// message arrives. Forced with a run refused for many issues at a laptop-sized window.
#include "../src/shell/editor_window.hpp"
#include "../src/shell/message_label.hpp"
#include "ui_test_storage.hpp"
#include <QAction>
#include <QApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>
#include <QToolButton>
#include <iostream>
#include <stdexcept>
#include <string>
using namespace multra;
namespace {
void require(bool ok,const std::string& message){if(!ok)throw std::runtime_error(message);}
void settle(){QApplication::processEvents();QTest::qWait(30);QApplication::processEvents();}
// The shipped four-leg without half its Connectors: a run is refused for many routes at once.
QString brokenProject(const std::filesystem::path& data,const QTemporaryDir& dir) {
    QFile in(QString::fromStdString((data/"projects/four-leg-signalised.traffic.json").string()));in.open(QIODevice::ReadOnly);
    auto root=QJsonDocument::fromJson(in.readAll()).object();auto network=root.value("network").toObject();
    auto connectors=network.value("connectors").toArray();while(connectors.size()>10)connectors.removeLast();
    network["connectors"]=connectors;root["network"]=network;
    const auto path=dir.filePath("broken.traffic.json");QFile out(path);out.open(QIODevice::WriteOnly);
    out.write(QJsonDocument(root).toJson());return path;
}
void check(const std::filesystem::path& data,const QString& language) {
    QTemporaryDir dir;
    EditorWindow window(data,language);window.resize(1366,768);window.show();settle();
    window.openFile(brokenProject(data,dir));settle();
    window.findChild<QAction*>("editorWorkspaceTables")->trigger();settle(); // isolate message height from opening Objects
    auto* message=dynamic_cast<MessageLabel*>(window.findChild<QWidget*>("editorError"));
    require(message,"The message line is not a MessageLabel");
    require(message->text().isEmpty() && !message->isVisible(),"An empty message line takes room");
    window.findChild<QAction*>("editorStep")->trigger();settle();
    const auto full=message->text();
    // The forcing: the refusal has more lines than the line shows.
    require(message->lines().size()>2,"The refused run produced "+std::to_string(message->lines().size())+" lines, not more than 2");
    require(message->isVisible(),"A refusal is not shown");
    const auto shown=message->shownText();
    require(!shown.contains('\n') && shown.contains(QString::number(message->lines().size()-1)),"The row does not say how many more");
    require(message->toolTip()==full,"The tooltip is not the whole message");
    const QFontMetrics metrics(message->font());
    require(message->height()<=metrics.lineSpacing()+16,"The message line is taller than one row: "+std::to_string(message->height()));
    const int withMessage=window.canvas()->height(),messageHeight=message->height();
    auto* close=message->findChild<QToolButton*>("editorMessageClose");
    require(close && close->isVisible(),"The message line has no close button");
    QTest::mouseClick(close,Qt::LeftButton);settle();
    require(!message->isVisible() && message->text()==full,"Close did not hide the line, or dropped the message");
    // Compare the close itself: Run feedback and opening Problems also change the layout.
    require(window.canvas()->height()>=withMessage&&window.canvas()->height()-withMessage<=messageHeight+16,
            "Closing one message row changed more than its row of canvas space");
    window.findChild<QAction*>("editorStep")->trigger();settle();
    require(!message->isVisible(),"The same message came back after it was closed");
    message->setText(QStringLiteral("A different message"));settle();
    require(message->isVisible() && message->lines().size()==1 && message->shownText()=="A different message",
            "A new single-line message is not shown whole");
    message->clear();settle();
    require(!message->isVisible(),"A cleared message line stays");
}
}
int main(int argc,char** argv) {
    QApplication app(argc,argv);useTestStorage();
    try {
        require(argc>1,"Data directory required");const std::filesystem::path data(argv[1]);
        for(const auto* language:{"en","th"})check(data,language);
        std::cout<<"Message line: one row, whole text kept, closable, en/th passed\n";
        return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
