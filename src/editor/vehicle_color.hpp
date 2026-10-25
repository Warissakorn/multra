#pragma once
#include "../model/network/display.hpp"
#include <cstdint>
#include <string_view>
namespace multra {
// A stable display-only choice: repaint, seeking and zoom never draw another colour.
// This deliberately consumes no simulation random numbers.
inline std::string vehicleDisplayColor(const DisplayCatalog& catalog,std::string_view type,
                                      std::uint64_t vehicle,const std::string& fallback) {
    const std::string id(type);
    if(const auto p=catalog.vehicleColorPalettes.find(id);p!=catalog.vehicleColorPalettes.end()&&!p->second.empty()) {
        std::uint64_t hash=1469598103934665603ULL;
        for(const unsigned char c:type){hash^=c;hash*=1099511628211ULL;}
        hash+=vehicle+0x9e3779b97f4a7c15ULL;
        hash=(hash^(hash>>30))*0xbf58476d1ce4e5b9ULL;
        hash=(hash^(hash>>27))*0x94d049bb133111ebULL;hash^=hash>>31;
        return p->second[hash%p->second.size()];
    }
    const auto c=catalog.vehicleColors.find(id);
    return c==catalog.vehicleColors.end()?fallback:c->second;
}
}
