#include "test.hpp"
#include "../src/project/object_csv.hpp"
#include "../src/project/document.hpp"
#include <locale>
#include <map>
using namespace multra;
namespace {
// Independent CSV reader for quoted commas, doubled quotes and embedded newlines.
std::vector<std::vector<std::string>> readCsv(const std::string& csv) {
    CHECK(csv.starts_with("\xEF\xBB\xBF"));
    std::vector<std::vector<std::string>> rows;std::vector<std::string> row;std::string cell;bool quoted=false;
    for(std::size_t i=3;i<csv.size();++i) {
        const char c=csv[i];
        if(c=='"') {
            if(quoted && i+1<csv.size() && csv[i+1]=='"'){cell+='"';++i;}else quoted=!quoted;
        }else if(!quoted && (c==',' || c=='\r')) {
            row.push_back(cell);cell.clear();
            if(c=='\r'){CHECK(i+1<csv.size() && csv[++i]=='\n');rows.push_back(row);row.clear();}
        }else cell+=c;
    }
    CHECK(!quoted);CHECK(row.empty());CHECK(cell.empty());return rows;
}
std::map<std::string,std::string> firstRow(const std::string& csv) {
    const auto rows=readCsv(csv);CHECK(rows.size()==2);CHECK(rows[0].size()==rows[1].size());
    std::map<std::string,std::string> out;
    for(std::size_t i=0;i<rows[0].size();++i)CHECK(out.emplace(rows[0][i],rows[1][i]).second);
    return out;
}
ProjectDocument fixture() {
    ProjectDocument d;Link link;link.id="link";link.name="ถนน, \"เหนือ\"\nสาย 1";
    link.geometry={{0,0},{3,4}};link.lanes={{"lane",3.123456789012345}};
    link.laneOffset=0.123456789012345;d.network.links.push_back(link);
    Connector c;c.id="connector";c.from={"link","lane",2.5};c.to={"link","lane",{}};
    c.geometry={{3,4},{6,8}};c.generated=true;d.network.connectors.push_back(c);
    d.definition.emplace();auto& def=*d.definition;
    def.routes.push_back({"route",{"link","connector"}});def.routeDestinations["route"]={"link",3.25};
    def.inputs.push_back({"input","route","car",123.456789012345,0,60});
    def.inputs[0].intervals={{0,60,123.456789012345}};
    def.routingDecisions.push_back({"decision","แยก",{{"route",2}},"link"});
    def.routingDecisions[0].position=0;def.routingDecisions[0].typeRules.push_back({"car",{2},{{3,4}}});
    return d;
}
struct Comma : std::numpunct<char> {char do_decimal_point() const override{return ',';}};
}
TEST(project, object_csv_exports_coordinates_lengths_nested_lanes_and_exact_text) {
    const auto d=fixture();const auto before=documentJson(d);const auto row=firstRow(objectTableCsv(d,ObjectTable::links));
    CHECK(row.at("id")=="link");CHECK(row.at("name")==d.network.links[0].name);
    CHECK(Json::parse(row.at("geometry"))==Json::array({Json{{"x",0},{"y",0}},Json{{"x",3},{"y",4}}}));
    CHECK(std::stod(row.at("referenceLength_m"))==5);CHECK(row.at("laneCount")=="1");
    CHECK(Json::parse(row.at("lanes"))[0].at("width")==d.network.links[0].lanes[0].width);
    CHECK(std::stod(row.at("laneOffset_m"))==d.network.links[0].laneOffset);
    CHECK(documentJson(d)==before);
}
TEST(project, object_csv_exports_connector_attachments_without_materializing_default_stations) {
    const auto d=fixture();const auto row=firstRow(objectTableCsv(d,ObjectTable::connectors));
    CHECK(row.at("fromLinkId")=="link");CHECK(row.at("fromLaneId")=="lane");
    CHECK(std::stod(row.at("fromStation_m"))==2.5);CHECK(row.at("toStation_m").empty());
    CHECK(row.at("generated")=="true");CHECK(std::stod(row.at("referenceLength_m"))==5);
    CHECK(Json::parse(row.at("laneWidths"))==Json::array());
}
TEST(project, object_csv_exports_authored_demand_routes_decisions_and_counted_convenience_values) {
    auto d=fixture();const auto input=firstRow(objectTableCsv(d,ObjectTable::inputs));
    CHECK(std::stod(input.at("vehiclesPerHour"))==d.definition->inputs[0].vehiclesPerHour);
    CHECK(Json::parse(input.at("intervals"))[0].at("vehiclesPerHour")==d.definition->inputs[0].vehiclesPerHour);
    const auto route=firstRow(objectTableCsv(d,ObjectTable::routes));
    CHECK(Json::parse(route.at("segmentIds"))==Json::array({"link","connector"}));
    CHECK(route.at("destinationObjectId")=="link");CHECK(std::stod(route.at("destinationStation_m"))==3.25);
    const auto decision=firstRow(objectTableCsv(d,ObjectTable::decisions));
    CHECK(decision.at("position_m")=="0.0");CHECK(Json::parse(decision.at("typeRules"))[0].at("intervalFlows")[0][1]==4);
    d.definition->inputs[0].volumeFromCounts=true;
    const auto counted=firstRow(objectTableCsv(d,ObjectTable::inputs));
    CHECK(counted.at("volumeFromCounts")=="true");CHECK(counted.at("intervals").empty());
}
TEST(project, object_csv_is_header_only_for_empty_tables_and_keeps_document_order) {
    for(auto table:{ObjectTable::links,ObjectTable::connectors,ObjectTable::inputs,ObjectTable::routes,ObjectTable::decisions}) {
        const auto rows=readCsv(objectTableCsv(ProjectDocument{},table));CHECK(rows.size()==1);CHECK(rows[0][0]=="id");
    }
    auto d=fixture();d.network.links.push_back(d.network.links[0]);d.network.links[0].id="z";d.network.links[1].id="a";
    const auto rows=readCsv(objectTableCsv(d,ObjectTable::links));CHECK(rows.size()==3);CHECK(rows[1][0]=="z");CHECK(rows[2][0]=="a");
}
TEST(project, object_csv_precision_is_independent_of_global_numeric_locale) {
    const auto d=fixture();const auto expected=objectTableCsv(d,ObjectTable::links);const auto previous=std::locale();
    std::locale::global(std::locale(previous,new Comma));
    std::string actual;
    try{actual=objectTableCsv(d,ObjectTable::links);}catch(...){std::locale::global(previous);throw;}
    std::locale::global(previous);CHECK(actual==expected);
}
