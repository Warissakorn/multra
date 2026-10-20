#include "../src/shell/legacy_storage.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <iostream>
#include <stdexcept>
using namespace trafficsim;
// D286: recovery files and map tiles written under the old application name move to the new
// one on first start, and a folder already under the new name is left alone.
namespace {
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
QString location(const QString& name,QStandardPaths::StandardLocation kind) {
    const auto current=QCoreApplication::applicationName();
    QCoreApplication::setApplicationName(name);
    const auto path=QStandardPaths::writableLocation(kind);
    QCoreApplication::setApplicationName(current);
    return path;
}
void write(const QString& path,const QByteArray& bytes) {
    QDir().mkpath(path.left(path.lastIndexOf('/')));
    QFile file(path);require(file.open(QIODevice::WriteOnly)&&file.write(bytes)==bytes.size(),"Fixture not written");
}
QByteArray read(const QString& path){QFile file(path);return file.open(QIODevice::ReadOnly)?file.readAll():QByteArray();}
void clear(const QString& legacy,const QString& current) {
    for(const auto& name:{legacy,current})
        for(const auto kind:{QStandardPaths::AppLocalDataLocation,QStandardPaths::CacheLocation})
            QDir(location(name,kind)).removeRecursively();
}
}
int main(int argc,char** argv) {
    QCoreApplication app(argc,argv);QStandardPaths::setTestModeEnabled(true);
    try {
        const QString legacy="LegacyStorageOld",current="LegacyStorageNew";
        QCoreApplication::setApplicationName(current);
        clear(legacy,current);
        const auto oldRecovery=location(legacy,QStandardPaths::AppLocalDataLocation)+"/recovery/a.traffic.json";
        const auto oldTile=location(legacy,QStandardPaths::CacheLocation)+"/map-tiles/1/2/3.png";
        write(oldRecovery,"recovered");write(oldTile,"tile");
        require(QFile::exists(oldRecovery)&&QFile::exists(oldTile),"Old folders were not created");
        moveLegacyStorage(legacy);
        require(read(location(current,QStandardPaths::AppLocalDataLocation)+"/recovery/a.traffic.json")=="recovered",
            "Recovery file did not move to the new name");
        require(read(location(current,QStandardPaths::CacheLocation)+"/map-tiles/1/2/3.png")=="tile",
            "Map tile did not move to the new name");
        require(!QFile::exists(oldRecovery)&&!QFile::exists(oldTile),"Old files left behind after the move");

        // A folder already under the new name wins: nothing is merged or overwritten.
        clear(legacy,current);
        const auto newRecovery=location(current,QStandardPaths::AppLocalDataLocation)+"/recovery/b.traffic.json";
        write(oldRecovery,"old");write(newRecovery,"new");
        require(QFile::exists(oldRecovery)&&QFile::exists(newRecovery),"Both folders were not created");
        moveLegacyStorage(legacy);
        require(read(newRecovery)=="new","The new folder was overwritten");
        require(read(oldRecovery)=="old","The old folder was touched although the new one existed");
        require(QCoreApplication::applicationName()==current,"The application name was not restored");
        clear(legacy,current);
        std::cout<<"Legacy storage moves once and never overwrites\n";
        return 0;
    } catch(const std::exception& error) {
        std::cerr<<error.what()<<'\n';
        return 1;
    }
}
