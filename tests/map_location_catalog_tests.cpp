#include "../src/editor/map_locations.hpp"
#include "../src/shell/path.hpp"
#include <QCoreApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <algorithm>
#include <cmath>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string_view>

using namespace multra;
namespace {
void require(bool condition,const char* message) {if(!condition)throw std::runtime_error(message);}
template<class Range> const auto& named(const Range& locations,std::string_view id) {
    const auto found=std::find_if(locations.begin(),locations.end(),[&](const auto& location){return location.id==id;});
    require(found!=locations.end(),"Expected location missing from catalog");return *found;
}
void installedCatalog(const std::filesystem::path& path) {
    const auto countries=loadMapLocations(path);require(countries.size()==239,"Bundled country coverage changed");
    std::size_t regions=0;for(const auto& country:countries)regions+=country.regions.size();
    require(regions==4588,"Bundled region coverage changed");
    const auto& thailand=named(countries,"TH");require(thailand.regions.size()==77,"Thai province coverage is not 77");
    require(mapLocationName(thailand,"th")==QString::fromUtf8("ประเทศไทย"),"Thai country label missing");
    require(mapLocationName(thailand,"en")=="Thailand","English country label missing");
    const auto& bangkok=named(thailand.regions,"TH-10");
    require(bangkok.name=="Bangkok" && mapLocationName(bangkok,"th")==QString::fromUtf8("กรุงเทพมหานคร"),
        "Bangkok inherited another province's native name");
    const auto& buengKan=named(thailand.regions,"TH-38");
    require(mapLocationName(buengKan,"th")==QString::fromUtf8("จังหวัดบึงกาฬ"),"Bueng Kan inherited Nong Khai's native name");
    require(bangkok.centre.longitude>100 && bangkok.centre.longitude<101 &&
        bangkok.centre.latitude>13 && bangkok.centre.latitude<14,"Bangkok representative point is outside Bangkok's vicinity");
    const auto& unitedStates=named(countries,"US");
    require(unitedStates.regions.size()>=50,"United States state choices missing");
    const auto& france=named(countries,"FR");
    require(france.name=="France" && france.centre.longitude>0 && france.centre.longitude<10,
        "A dependency replaced the country reference");
    require(named(named(countries,"NO").regions,"NO-BVT-00").name=="Bouvet Island",
        "The source's exceptional Bouvet identifier was not normalized");
}
QJsonObject fixture() {
    return QJsonDocument::fromJson(R"({"schemaVersion":1,"source":{},"countries":[
        {"id":"TH","name":"Thailand","longitude":100,"latitude":15,"regions":[]}
    ]})").object();
}
void write(const QString& path,const QByteArray& bytes) {
    QFile file(path);require(file.open(QIODevice::WriteOnly),"Catalog fixture could not be written");
    require(file.write(bytes)==bytes.size(),"Catalog fixture write was incomplete");
}
void rejects(const QString& path,const QByteArray& bytes) {
    write(path,bytes);bool rejected=false;
    try {(void)loadMapLocations(nativePath(path));}
    catch(const std::invalid_argument& error) {rejected=std::string(error.what())=="EDIT_MAP_LOCATIONS";}
    require(rejected,"Malformed location catalog accepted");
}
void invalidCatalogs() {
    QTemporaryDir directory;require(directory.isValid(),"Catalog fixture directory failed");
    const auto path=directory.filePath("locations.json");const auto valid=fixture();
    write(path,QJsonDocument(valid).toJson());const auto countryOnly=loadMapLocations(nativePath(path));
    require(countryOnly.size()==1 && countryOnly.front().regions.empty(),"Country-only choice required a province");
    require(countryOnly.front().centre.longitude==100 && countryOnly.front().centre.latitude==15,"Country centre changed");
    require(mapLocationName(countryOnly.front(),"th")=="Thailand","Absent Thai label did not fall back to English");
    const auto alterCountry=[&](const std::function<void(QJsonObject&)>& change) {
        auto root=valid;auto entries=root["countries"].toArray();auto country=entries[0].toObject();
        change(country);entries[0]=country;root["countries"]=entries;rejects(path,QJsonDocument(root).toJson());
    };
    alterCountry([](auto& c){c["latitude"]=85;});
    alterCountry([](auto& c){c["latitude"]=-81;});
    alterCountry([](auto& c){c["longitude"]=181;});
    alterCountry([](auto& c){c["longitude"]="100";});
    alterCountry([](auto& c){c["latitude"]=QJsonValue();});
    alterCountry([](auto& c){c["id"]="th";});
    alterCountry([](auto& c){c["name"]=" ";});
    alterCountry([](auto& c){c["nameTh"]=7;});
    alterCountry([](auto& c){c["regions"]=QJsonObject();});
    alterCountry([](auto& c){c["surprise"]=1;});
    auto duplicate=valid;auto countries=duplicate["countries"].toArray();countries.append(countries[0]);
    duplicate["countries"]=countries;rejects(path,QJsonDocument(duplicate).toJson());
    const QJsonObject region{{"id","TH-10"},{"name","Bangkok"},{"longitude",100.5},{"latitude",13.7}};
    alterCountry([&](auto& c){c["regions"]=QJsonArray{region,region};});
    alterCountry([&](auto& c){auto r=region;r["latitude"]=90;c["regions"]=QJsonArray{r};});
    auto future=valid;future["schemaVersion"]=2;rejects(path,QJsonDocument(future).toJson());
    rejects(path,"not JSON");rejects(path,"[]");rejects(path,QByteArray(2*1024*1024+1,' '));
    require(QFile::remove(path),"Missing-catalog forcing failed");bool missing=false;
    try {(void)loadMapLocations(nativePath(path));}catch(const std::invalid_argument&) {missing=true;}
    require(missing,"Missing location catalog was accepted");
}
}
int main(int argc,char** argv) {
    QCoreApplication app(argc,argv);
    try {
        require(argc==2,"Expected bundled map location catalog path");
        installedCatalog(nativePath(app.arguments().at(1)));invalidCatalogs();
        std::cout<<"map location catalog checks passed\n";return 0;
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
