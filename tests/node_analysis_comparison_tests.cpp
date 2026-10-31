#include "node_model_demand_fixture.hpp"
#include "../src/project/node_analysis_fill.hpp"
#include "../src/project/node_analysis_comparison.hpp"
#include "../src/project/evaluation.hpp"
#include "../src/project/csv_format.hpp"
#include <limits>
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
void manualCompare(DrivingSide side) {
    auto d=nodeModelTest::fixture(side);const auto id=d.network.nodes[0].id;
    const NodeModelDemandRequest window{30,90,{}};
    auto counts=analysisFromModel(*d.network.nodes[0].analysis,previewNodeModelDemand(documentJson(d).dump(),id,window,test::root()/"data"),window);
    counts.modelSource.reset();setNodeAnalysis(d,id,counts);changeEvaluationPeriod(d,30,90,30);
    CHECK(!d.network.nodes[0].analysis->modelSource&&counts.turns.size()==12&&counts.analysisPeriod==60./3600);
    const auto before=documentJson(d);
    const auto s=compileDocument(d,test::root()/"data");const auto spec=evaluationSpec(d,s,test::root()/"data");
    const auto runs=runSeeds(s.scenario,spec,{42,43,44});const auto b=aggregate(runs);
    CHECK(b.seeds.size()==3&&runs.front().report.time>=150);
    const auto a=analyseNodeAnalysisInput(before.dump(),test::root()/"data");const auto source=nodeAnalysisMicroSource(d,s.network);
    CHECK(a.run.nodes.size()==1&&!a.run.nodes[0].modelSource&&a.run.nodes[0].analysisPeriod*3600==60);
    const auto result=compareNodeAnalysis(a,source,b,runs.front().report,{{id,30,90}});
    CHECK(result.rows.size()==5&&result.issues.empty());
    for(const auto& r:result.rows)CHECK(r.manualCounts&&r.nodeId==id);
    CHECK(result.rows[0].microDelay==b.nodes[0].delay&&result.rows[0].analyticalDelay==a.run.nodes[0].delay);
    const auto csv=nodeAnalysisComparisonCsv(result);CHECK(csv.find("countsSource")!=std::string::npos&&csv.find(",\"manual counts\"\n")!=std::string::npos);
    const auto noWindow=compareNodeAnalysis(a,source,b,runs.front().report);
    CHECK(noWindow.rows.empty()&&junctionTest::issue(noWindow.issues,"NODE_MICRO_NO_WINDOW",id));
    const auto wrongNode=compareNodeAnalysis(a,source,b,runs.front().report,{{"another-node",30,90}});
    CHECK(wrongNode.rows.empty()&&junctionTest::issue(wrongNode.issues,"NODE_MICRO_NO_WINDOW",id));
    const auto duplicate=compareNodeAnalysis(a,source,b,runs.front().report,{{id,30,90},{id,30,90}});
    CHECK(duplicate.rows.empty()&&junctionTest::issue(duplicate.issues,"NODE_MICRO_WINDOW",id));
    for(const auto& bad:std::vector<NodeAnalysisManualWindow>{{id,31,91},{id,30,89},{id,-1,59},{id,90,30},{id,std::numeric_limits<double>::quiet_NaN(),90},{id,30,std::numeric_limits<double>::infinity()}}) {
        CHECK(bad.start!=30||bad.end!=90);const auto refused=compareNodeAnalysis(a,source,b,runs.front().report,{bad});
        CHECK(refused.rows.empty()&&junctionTest::issue(refused.issues,"NODE_MICRO_WINDOW",id));
    }
    auto other=d;other.network.nodes[0].analysis->analysisPeriod=.25;
    const auto wrongT=analyseNodeAnalysisInput(documentJson(other).dump(),test::root()/"data");CHECK(wrongT.run.nodes[0].analysisPeriod*3600!=60);
    const auto refused=compareNodeAnalysis(wrongT,nodeAnalysisMicroSource(other,s.network),b,runs.front().report,{{id,30,90}});
    CHECK(refused.rows.empty()&&junctionTest::issue(refused.issues,"NODE_MICRO_WINDOW",id));
    auto stale=source;++stale.document.revision;CHECK(stale.document!=a.source);
    const auto mismatch=compareNodeAnalysis(a,stale,b,runs.front().report,{{id,30,90}});CHECK(mismatch.rows.empty()&&junctionTest::issue(mismatch.issues,"NODE_MICRO_SNAPSHOT"));
    CHECK(documentJson(d)==before); // confirmation/CSV never becomes a stored field or Command
}
}
TEST(nodeanalysis,na28_na29_comparison_left){compare(DrivingSide::left);}
TEST(nodeanalysis,na28_na29_comparison_right){compare(DrivingSide::right);}
TEST(nodeanalysis,na28_na30_manual_window_left){manualCompare(DrivingSide::left);}
TEST(nodeanalysis,na28_na30_manual_window_right){manualCompare(DrivingSide::right);}
