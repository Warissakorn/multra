#include "catalog_commands.hpp"
#include "demand_commands.hpp"
#include "../project/od_demand.hpp"
#include <algorithm>
#include <stdexcept>
namespace trafficsim {
void putDemandCatalog(ProjectDocument& d,DemandCatalog catalog) {
    const bool named=namesMode(d.zones);
    auto& target=demand(d);
    target.vehicleTypes=std::move(catalog.vehicleTypes);target.behaviours=std::move(catalog.behaviours);
    target.compositions=std::move(catalog.compositions);target.vehicleTypeNames=std::move(catalog.vehicleTypeNames);
    target.externalVehicleTypes=false;target.externalBehaviours=false;target.externalCompositions=false;
    if(named) {target.modes=std::move(catalog.modes);target.externalModes=false;}
}
void ownModesWhenNamed(ProjectDocument& d,std::vector<Mode> modes) {
    if(!d.definition || d.definition->externalCompositions || !d.definition->externalModes || !namesMode(d.zones))return;
    d.definition->modes=std::move(modes);d.definition->externalModes=false;
}
std::string putMode(ProjectDocument& d,Mode mode) {
    auto& target=demand(d);
    if(target.externalModes)throw std::invalid_argument("EDIT_MODES_NOT_OWNED");
    if(mode.id.empty())mode.id=allocateId(d,"mode");
    const auto id=mode.id;
    auto& modes=demand(d).modes;
    const auto found=std::find_if(modes.begin(),modes.end(),[&](const auto& m){return m.id==id;});
    if(found!=modes.end())*found=std::move(mode);else modes.push_back(std::move(mode));
    return id;
}
void deleteMode(ProjectDocument& d,const std::string& id) {
    for(const auto& m:d.zones.odMatrices)if(m.modeId==id)throw std::invalid_argument("EDIT_MODE_IN_USE");
    if(!d.definition || !std::erase_if(d.definition->modes,[&](const auto& m){return m.id==id;}))throw std::invalid_argument("EDIT_UNKNOWN_OBJECT");
}
}
