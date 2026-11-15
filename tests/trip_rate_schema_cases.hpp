#pragma once
#include "demand_schema_cases.hpp"
#include <functional>

// D570: the root tripRateSets (M10.2 G2), refused below 66 even null. A set is a closed
// {id, name, source, unit, purposes}; `source` a closed {citation, table} of optional text; a purpose
// a closed {id, productions, attractions}; each side a closed row chosen by `method`:
// crossClassification {method, categories[>=1 of {attribute, rate >= 0}]} or
// regression {method, constant, terms[{attribute, coefficient}]}. Blank text, unique ids and
// attributes stay with the codec, so those cases are labelled accepted by the schema.
namespace tripRateCases {
struct Case {std::string name;multra::Json document;bool schemaAccepts;};
inline constexpr int from=66;
inline multra::Json cross() {
    return {{"method","crossClassification"},{"categories",multra::Json::array({multra::Json{{"attribute","hh.cars0"},{"rate",0.9}}})}};
}
inline multra::Json regression() {
    return {{"method","regression"},{"constant",0},{"terms",multra::Json::array({multra::Json{{"attribute","jobs"},{"coefficient",1.1}}})}};
}
inline multra::Json set() {
    return {{"id","own"},{"name","Own set"},{"source",multra::Json{{"citation","c"},{"table","t"}}},{"unit","person trips per day"},
        {"purposes",multra::Json::array({multra::Json{{"id","HBW"},{"productions",cross()},{"attractions",regression()}}})}};
}
inline multra::Json minimal(int version=from) {
    auto j=demandCases::minimal(version);
    if(version>=from)j["tripRateSets"]=multra::Json::array({set()});
    return multra::Json::parse(j.dump());
}
inline std::vector<Case> cases() {
    using namespace multra;
    std::vector<Case> result;
    for(int v=1;v<=kLatestSchema;++v) {
        const auto base=minimal(v);
        const auto add=[&](const std::string& name,const std::function<void(Json&)>& edit,bool valid) {
            auto j=base;edit(j);result.push_back({name,Json::parse(j.dump()),valid});
        };
        result.push_back({"minimal",base,true});
        add("tripRateSets null",[](Json& j){j["tripRateSets"]=nullptr;},false);
        if(v<from) {add("tripRateSets below 66",[](Json& j){j["tripRateSets"]=Json::array({set()});},false);continue;}
        const auto at=[](Json& j)->Json& {return j["tripRateSets"][0];};
        add("tripRateSets []",[](Json& j){j["tripRateSets"]=Json::array();},false);
        add("tripRateSets 7",[](Json& j){j["tripRateSets"]=7;},false);
        add("set 7",[](Json& j){j["tripRateSets"][0]=7;},false);
        add("set extra",[&](Json& j){at(j)["extra"]=1;},false);
        for(const auto* key:{"id","name","source","unit","purposes"}) {
            add(std::string("set missing ")+key,[&,key](Json& j){at(j).erase(key);},false);
            add(std::string("set ")+key+" null",[&,key](Json& j){at(j)[key]=nullptr;},false);
            add(std::string("set ")+key+" 7",[&,key](Json& j){at(j)[key]=7;},false);
        }
        add("set id blank",[&](Json& j){at(j)["id"]=" ";},true);add("set name empty",[&](Json& j){at(j)["name"]="";},true);
        add("source {}",[&](Json& j){at(j)["source"]=Json::object();},true);
        add("source extra",[&](Json& j){at(j)["source"]["extra"]=1;},false);
        for(const auto* key:{"citation","table"}) {
            add(std::string("source ")+key+" null",[&,key](Json& j){at(j)["source"][key]=nullptr;},false);
            add(std::string("source ")+key+" 7",[&,key](Json& j){at(j)["source"][key]=7;},false);
            add(std::string("source without ")+key,[&,key](Json& j){at(j)["source"].erase(key);},true);
        }
        add("two sets, one id",[&](Json& j){j["tripRateSets"].push_back(set());},true); // DUPLICATE_ID: the codec's
        add("purposes []",[&](Json& j){at(j)["purposes"]=Json::array();},false);
        add("purposes 7",[&](Json& j){at(j)["purposes"]=7;},false);
        add("purpose 7",[&](Json& j){at(j)["purposes"][0]=7;},false);
        add("purpose extra",[&](Json& j){at(j)["purposes"][0]["extra"]=1;},false);
        add("two purposes, one id",[&](Json& j){at(j)["purposes"].push_back(at(j)["purposes"][0]);},true);
        for(const auto* key:{"id","productions","attractions"}) {
            add(std::string("purpose missing ")+key,[&,key](Json& j){at(j)["purposes"][0].erase(key);},false);
            add(std::string("purpose ")+key+" null",[&,key](Json& j){at(j)["purposes"][0][key]=nullptr;},false);
            add(std::string("purpose ")+key+" 7",[&,key](Json& j){at(j)["purposes"][0][key]=7;},false);
        }
        for(const auto* side:{"productions","attractions"}) {
            const auto s=[&,side](const std::string& name,const std::function<void(Json&)>& edit,bool valid) {
                add(std::string(side)+" "+name,[&,side](Json& j){edit(at(j)["purposes"][0][side]);},valid);
            };
            s("{}",[](Json& x){x=Json::object();},false);
            s("method null",[](Json& x){x["method"]=nullptr;},false);s("method 7",[](Json& x){x["method"]=7;},false);
            s("method unknown",[](Json& x){x["method"]="gravity";},false);
            s("method missing",[](Json& x){x.erase("method");},false);
            s("extra",[](Json& x){x["extra"]=1;},false);
            // A cross-classification row: categories, no constant or terms.
            s("cross",[](Json& x){x=cross();},true);s("regression",[](Json& x){x=regression();},true);
            s("cross with constant",[](Json& x){x=cross();x["constant"]=0;},false);
            s("cross with terms",[](Json& x){x=cross();x["terms"]=Json::array();},false);
            s("regression with categories",[](Json& x){x=regression();x["categories"]=Json::array();},false);
            s("cross missing categories",[](Json& x){x=cross();x.erase("categories");},false);
            s("cross categories []",[](Json& x){x=cross();x["categories"]=Json::array();},false);
            s("cross categories 7",[](Json& x){x=cross();x["categories"]=7;},false);
            s("category 7",[](Json& x){x=cross();x["categories"][0]=7;},false);
            s("category extra",[](Json& x){x=cross();x["categories"][0]["extra"]=1;},false);
            s("category attribute missing",[](Json& x){x=cross();x["categories"][0].erase("attribute");},false);
            s("category attribute 7",[](Json& x){x=cross();x["categories"][0]["attribute"]=7;},false);
            s("category attribute blank",[](Json& x){x=cross();x["categories"][0]["attribute"]="";},true);
            s("category rate missing",[](Json& x){x=cross();x["categories"][0].erase("rate");},false);
            s("category rate null",[](Json& x){x=cross();x["categories"][0]["rate"]=nullptr;},false);
            s("category rate text",[](Json& x){x=cross();x["categories"][0]["rate"]="1";},false);
            s("category rate 0",[](Json& x){x=cross();x["categories"][0]["rate"]=0;},true);
            s("category rate -0.1",[](Json& x){x=cross();x["categories"][0]["rate"]=-0.1;},false);
            s("category repeats an attribute",[](Json& x){x=cross();x["categories"].push_back(x["categories"][0]);},true);
            s("regression missing constant",[](Json& x){x=regression();x.erase("constant");},false);
            s("regression constant null",[](Json& x){x=regression();x["constant"]=nullptr;},false);
            s("regression constant text",[](Json& x){x=regression();x["constant"]="0";},false);
            s("regression constant negative",[](Json& x){x=regression();x["constant"]=-5;},true);
            s("regression missing terms",[](Json& x){x=regression();x.erase("terms");},false);
            s("regression terms []",[](Json& x){x=regression();x["terms"]=Json::array();},true);
            s("regression terms 7",[](Json& x){x=regression();x["terms"]=7;},false);
            s("term 7",[](Json& x){x=regression();x["terms"][0]=7;},false);
            s("term extra",[](Json& x){x=regression();x["terms"][0]["extra"]=1;},false);
            s("term attribute missing",[](Json& x){x=regression();x["terms"][0].erase("attribute");},false);
            s("term attribute null",[](Json& x){x=regression();x["terms"][0]["attribute"]=nullptr;},false);
            s("term coefficient missing",[](Json& x){x=regression();x["terms"][0].erase("coefficient");},false);
            s("term coefficient text",[](Json& x){x=regression();x["terms"][0]["coefficient"]="1";},false);
            s("term coefficient negative",[](Json& x){x=regression();x["terms"][0]["coefficient"]=-2;},true);
        }
    }
    return result;
}
}
