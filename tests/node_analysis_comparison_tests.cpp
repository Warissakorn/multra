#include "node_model_demand_fixture.hpp"
#include "../src/project/node_analysis_fill.hpp"
#include "../src/project/node_analysis_comparison.hpp"
#include "../src/project/evaluation.hpp"
#include "../src/project/csv_format.hpp"
using namespace multra;
namespace {
void compare(DrivingSide side) {
    auto d=nodeModelTest::fixture(side);const auto id=d.network.nodes[0].id;
    const NodeModelDemandRequest window{30,90,{"rigid-truck"}};
    const auto volumes=previewNodeModelDemand(documentJson(d).dump(),id,window,test::root()/"data");
    setNodeAnalysis(d,id,analysisFromModel(*d.network.nodes[0].analysis,volumes,window));
    changeEvaluationPeriod(d,30,90,30);
    d.network.nodes[0].name="Same, \"Node\"";
    putNode(d,{"","Same, \"Node\"",{{990,-10},{1010,-10},{1010,10},{990,10}},false});
    std::rotate(d.network.nodes.begin(),d.network.nodes.end()-1,d.network.nodes.end());
    const auto s=compileDocument(d,test::root()/"data");const auto spec=evaluationSpec(d,s,test::root()/"data");
    const auto runs=runSeeds(s.scenario,spec,{42,43,44});const auto batch=aggregate(runs);
    CHECK(batch.seeds.size()==3&&batch.nodes.size()==2&&batch.nodes[0].name==batch.nodes[1].name);
    CHECK(runs.front().report.time>=150); // real completed batch; a seed with no completed movement keeps its blank delay
    const auto analysis=analyseNodeAnalysisInput(documentJson(d).dump(),test::root()/"data");CHECK(analysis.run.nodes.size()==1);
    const auto source=nodeAnalysisMicroSource(d,s.network);
    const auto result=compareNodeAnalysis(analysis,source,batch,runs.front().report);
    CHECK(result.issues.empty()&&result.rows.size()==5&&result.seeds==batch.seeds&&result.start==30&&result.end==90);
    const auto& node=result.rows[0];CHECK(node.nodeId==id&&node.inLink.empty());
    CHECK(node.analyticalDelay==analysis.run.nodes[0].delay&&node.microDelay==batch.nodes[1].delay&&node.unfinished==batch.nodes[1].unfinished);
    for(std::size_t k=0;k<4;++k) {
        const auto& row=result.rows[k+1];CHECK(row.inLink==source.keys[1].inLinks[k]&&row.microDelay==batch.nodes[1].legs[k].delay&&row.unfinished==batch.nodes[1].legs[k].unfinished);
    }
    const auto csv=nodeAnalysisComparisonCsv(result);CHECK(csv.starts_with("# MULTRA - not yet validated.")&&csv.find(csvQuoted(d.network.nodes[1].name))!=std::string::npos);
    CHECK(nodeAnalysisComparisonCsv(result,true).find("project")!=std::string::npos);
    auto other=source;other.document.revision+=1;CHECK(other.document!=analysis.source);
    const auto stale=compareNodeAnalysis(analysis,other,batch,runs.front().report);CHECK(stale.rows.empty()&&junctionTest::issue(stale.issues,"NODE_MICRO_SNAPSHOT"));
    auto period=runs.front().report;period.evaluationEnd=91;CHECK(period.evaluationEnd!=window.end);
    const auto mismatch=compareNodeAnalysis(analysis,source,batch,period);CHECK(mismatch.rows.empty()&&junctionTest::issue(mismatch.issues,"NODE_MICRO_WINDOW"));
    period=runs.front().report;period.time=50;CHECK(period.time<d.definition->duration);
    const auto partial=compareNodeAnalysis(analysis,source,batch,period);CHECK(partial.rows.empty()&&junctionTest::issue(partial.issues,"NODE_MICRO_UNFINISHED"));
    auto manual=analysis;manual.run.nodes[0].modelSource.reset();CHECK(!manual.run.nodes[0].modelSource);
    const auto unconfirmed=compareNodeAnalysis(manual,source,batch,runs.front().report);CHECK(unconfirmed.rows.empty()&&junctionTest::issue(unconfirmed.issues,"NODE_MICRO_NO_WINDOW"));
}
}
TEST(nodeanalysis,na28_na29_comparison_left){compare(DrivingSide::left);}
TEST(nodeanalysis,na28_na29_comparison_right){compare(DrivingSide::right);}
