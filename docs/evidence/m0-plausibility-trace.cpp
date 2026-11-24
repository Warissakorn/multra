// M0 plausibility evidence (D369): per-tick vehicle trace of a bare M0 scenario. Not a product tool.
// Build from the repository root against a desktop build (see m0-plausibility.md).
#include "../../src/core/simulation.hpp"
#include "../../src/project/load.hpp"
#include <cstdio>
#include <string>
using namespace multra;
int main(int argc,char**argv){
  auto ls=loadScenario(argv[1],argv[2]);
  // Optional (D653's look under project rules): argv[4] gives every behaviour that amberDeceleration,
  // argv[5] sets clampRules.lineRest ("1" on, else off). Without them the bare M0 scenario is unchanged.
  if(argc>4) for(auto&b:ls.scenario.behaviours) b.amberDeceleration=std::stod(argv[4]);
  if(argc>5) ls.scenario.clampRules.lineRest=std::string(argv[5])=="1";
  auto st=createSimulation(ls.scenario,argc>3?std::stoul(argv[3]):42);
  auto ticks=totalTicks(ls.scenario);
  std::printf("t,id,route,dist,speed,acc\n");
  while(st.tick<ticks){ st=stepSimulation(std::move(st));
    for(auto&v:st.vehicles) std::printf("%.1f,%llu,%u,%.3f,%.3f,%.3f\n",st.time,(unsigned long long)v.id,v.routeIndex,v.distance,v.speed,v.acceleration);}
  std::fprintf(stderr,"completed %llu\n",(unsigned long long)st.completed);
}
