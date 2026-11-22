// D574: the schema half of every slice's labelled cases. With MULTRA_SCHEMA_LABELS set, writes each
// case's slice, name, document and schemaAccepts label as one JSON array for
// tools/check_schema_labels.py, which checks them against multra-project-schema's output
// (ctest schema-labels, registered when Python's jsonschema is found). Without it the test is a no-op.
// envelopeCases is not here: its labels are for projectEnvelopeSchema() alone (`network: {}` is
// accepted there and refused by the composed schema); project_envelope_tests.cpp checks them.
#include "test.hpp"
#include "behaviour_class_schema_cases.hpp"
#include "behaviour_schema_cases.hpp"
#include "composition_schema_cases.hpp"
#include "connector_schema_cases.hpp"
#include "cost_matrix_schema_cases.hpp"
#include "demand_schema_cases.hpp"
#include "deterrence_schema_cases.hpp"
#include "macro_run_schema_cases.hpp"
#include "map_schema_cases.hpp"
#include "measurement_schema_cases.hpp"
#include "mode_choice_schema_cases.hpp"
#include "mode_schema_cases.hpp"
#include "network_schema_cases.hpp"
#include "period_factor_schema_cases.hpp"
#include "right_of_way_schema_cases.hpp"
#include "routing_schema_cases.hpp"
#include "signal_control_schema_cases.hpp"
#include "signal_head_schema_cases.hpp"
#include "study_schema_cases.hpp"
#include "trip_rate_schema_cases.hpp"
#include "vehicle_type_schema_cases.hpp"
#include "zone_schema_cases.hpp"
#include <cstdlib>
#include <fstream>

using namespace multra;
TEST(schemalabels, dump_every_labelled_case) {
    const char* out=std::getenv("MULTRA_SCHEMA_LABELS");
    if(!out)return;
    // Streamed one case at a time, each moved into its record: the same bytes as one dumped array,
    // without a second copy of every document (195 MB, 2.2 GB peak before).
    std::ofstream file(out,std::ios::binary);
    CHECK(static_cast<bool>(file));
    bool first=true;
    file<<'[';
    const auto add=[&](const char* slice,auto cases) {
        for(auto& c:cases) {
            Json record=Json::object();
            record["slice"]=slice;record["name"]=std::move(c.name);record["document"]=std::move(c.document);record["accepts"]=c.schemaAccepts;
            file<<(first?"":",")<<record.dump();first=false;
        }
    };
    add("behaviourClassCases",behaviourClassCases::cases());
    add("behaviourCases",behaviourCases::cases());
    add("compositionCases",compositionCases::cases());
    add("connectorCases",connectorCases::cases());
    add("costMatrixCases",costMatrixCases::cases());
    add("demandCases",demandCases::cases());
    add("deterrenceCases",deterrenceCases::cases());
    add("macroRunCases",macroRunCases::cases());
    add("mapCases",mapCases::cases());
    add("measurementCases",measurementCases::cases());
    add("modeChoiceCases",modeChoiceCases::cases());
    add("modeCases",modeCases::cases());
    add("networkCases",networkCases::cases());
    add("periodFactorCases",periodFactorCases::cases());
    add("rightOfWayCases",rightOfWayCases::cases());
    add("routingCases",routingCases::cases());
    add("signalControlCases",signalControlCases::cases());
    add("signalHeadCases",signalHeadCases::cases());
    add("studyCases",studyCases::cases());
    add("tripRateCases",tripRateCases::cases());
    add("vehicleTypeCases",vehicleTypeCases::cases());
    add("zoneCases",zoneCases::cases());
    file<<']';
    CHECK(static_cast<bool>(file));
}
