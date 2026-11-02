#include "editor_window.hpp"
#include "form_values.hpp"
#include "../editor/ui_design_tokens.hpp"
#include "../commands/demand_commands.hpp"
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QPushButton>
#include <QTableWidget>
#include <QSpinBox>
#include <QTimeEdit>
#include <QVBoxLayout>

namespace multra {
namespace {
QDoubleSpinBox* number(QDialog& dialog,double min,double max,double value,const char* name) {
    auto* field=new QDoubleSpinBox(&dialog);field->setObjectName(name);field->setDecimals(3);
    field->setRange(min,max);setFormNumber(field,value);return field;
}
}
void EditorWindow::editRunSettings() {
    const auto def=history_.document().definition.value_or(AuthoringDefinition{});
    FormDialog dialog(this);dialog.setObjectName("editorSettingsDialog");dialog.setWindowTitle(text("editorRunSettings"));
    auto* form=new QFormLayout(&dialog);
    form->setContentsMargins(editorDesign::space3,editorDesign::space3,editorDesign::space3,editorDesign::space3);
    form->setHorizontalSpacing(editorDesign::space2);form->setVerticalSpacing(editorDesign::space1);
    auto* duration=number(dialog,0.001,10000000,def.duration,"editorDuration");
    auto* dt=number(dialog,0.001,0.5,def.timeStep,"editorTimeStep");
    // M5.3: the warm-up before results are counted; 0 with no end clears the period.
    const auto period=def.evaluation.value_or(EvaluationPeriod{});
    auto* warmup=number(dialog,0,10000000,period.warmup,"editorWarmup");
    // M5.9: the cool-down after the demand ends; validation keeps it on the time grid.
    auto* cooldown=number(dialog,0,10000000,period.cooldown,"editorCooldown");
    form->addRow(text("editorDuration"),duration);form->addRow(text("editorTimeStep"),dt);form->addRow(text("editorWarmup"),warmup);
    form->addRow(text("editorCooldown"),cooldown);
    // S1.1 (STUDY_PERIODS §4): the clock time of run time 0; it places every study period.
    auto* clock=new QTimeEdit(QTime(0,0).addSecs(static_cast<int>(def.clockStart)%86400),&dialog);clock->setObjectName("editorClockStart");
    clock->setDisplayFormat("HH:mm:ss");form->addRow(text("editorClockStart"),clock);
    const QTime clockShown=clock->time();
    // M3.2.8a.1 (MERGE_CLAMPS §3, D251; JOIN_ANTICIPATION §5, D255; LINE_APPROACH §7, D258): the clamp rules, labelled with their keys like the behaviour rows.
    const auto rule=[&](const char* key,const char* name,bool on){auto* box=new QCheckBox(QString::fromUtf8(key),&dialog);box->setObjectName(name);
        box->setChecked(on);form->addRow(box);return box;};
    auto* leaderTravel=rule("leaderTravel","editorLeaderTravel",def.clampRules.leaderTravel);
    auto* sideEntry=rule("sideEntryAtLine","editorSideEntryAtLine",def.clampRules.sideEntryAtLine);
    auto* redLineStop=rule("redLineStop","editorRedLineStop",def.clampRules.redLineStop);
    auto* joinAnticipation=rule("joinAnticipation","editorJoinAnticipation",def.clampRules.joinAnticipation);
    auto* standingStopFloor=rule("standingStopFloor","editorStandingStopFloor",def.clampRules.standingStopFloor);
    // S2.4 (MACRO_NETWORK §7, D277): the Macro target gap in % and the iteration cap; defaults write nothing.
    const auto macro=def.macroSettings.value_or(kMacroDefaults);
    auto* gap=number(dialog,0.0001,99,macro.targetGap*100,"editorMacroTargetGap");gap->setDecimals(4);setFormNumber(gap,macro.targetGap*100);
    auto* cap=new QSpinBox(&dialog);cap->setObjectName("editorMacroMaxIterations");cap->setRange(1,100000);cap->setValue(macro.maxIterations);
    form->addRow(text("editorMacroTargetGap"),gap);form->addRow(text("editorMacroMaxIterations"),cap);
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&dialog);form->addRow(buttons);
    buttons->button(QDialogButtonBox::Ok)->setText(text("editorConfirm"));buttons->button(QDialogButtonBox::Cancel)->setText(text("editorCancel"));
    validateDialog(dialog,buttons,[&]{commitChange("editorRunSettings",[&](auto& d){
        const MacroSettings chosen{formNumber(gap)==macro.targetGap*100?macro.targetGap:formNumber(gap)/100,cap->value()};
        if(!(chosen==macro))changeMacroSettings(d,chosen==kMacroDefaults?std::nullopt:std::optional(chosen));
        changeRunSettings(d,formNumber(duration),formNumber(dt));
        changeEvaluationPeriod(d,formNumber(warmup),period.end,formNumber(cooldown));
        if(clock->time()!=clockShown)demand(d).clockStart=QTime(0,0).secsTo(clock->time()); // untouched keeps a value past midnight
        changeClampRules(d,{leaderTravel->isChecked(),sideEntry->isChecked(),redLineStop->isChecked(),joinAnticipation->isChecked(),standingStopFloor->isChecked()});});},[this](const char* key){return text(key);});
    dialog.exec();
}
void EditorWindow::editProgram(const std::string& id) {
    SignalProgram value{id,0,{{30,SignalColor::red},{30,SignalColor::green},{3,SignalColor::amber}}};
    if(history_.document().definition)for(const auto& p:history_.document().definition->signalPrograms)if(p.id==id)value=p;
    FormDialog dialog(this);dialog.setObjectName("editorProgramDialog");dialog.setWindowTitle(text("editorEditProgram"));
    auto* layout=new QVBoxLayout(&dialog);layout->setContentsMargins(editorDesign::space3,editorDesign::space3,editorDesign::space3,editorDesign::space3);layout->setSpacing(editorDesign::space2);
    auto* form=new QFormLayout;form->setContentsMargins(0,0,0,0);form->setHorizontalSpacing(editorDesign::space2);form->setVerticalSpacing(editorDesign::space1);layout->addLayout(form);
    auto* offset=number(dialog,-10000000,10000000,value.offset,"editorProgramOffset");form->addRow(text("editorProgramOffset"),offset);
    auto* phases=new QTableWidget(0,2,&dialog);phases->setObjectName("editorPhases");
    phases->setHorizontalHeaderLabels({text("editorPhaseDuration"),text("editorPhaseColor")});
    phases->horizontalHeader()->setStretchLastSection(true);layout->addWidget(phases);
    const auto append=[&](SignalPhase phase){
        const int row=phases->rowCount();phases->insertRow(row);
        phases->setCellWidget(row,0,number(dialog,0.001,10000000,phase.duration,"editorPhaseDuration"));
        auto* color=new QComboBox(phases);color->addItem(text("editorRed"),static_cast<int>(SignalColor::red));
        color->addItem(text("editorAmber"),static_cast<int>(SignalColor::amber));color->addItem(text("editorGreen"),static_cast<int>(SignalColor::green));
        color->setCurrentIndex(color->findData(static_cast<int>(phase.color)));phases->setCellWidget(row,1,color);
    };
    for(const auto& phase:value.phases)append(phase);
    auto* add=new QPushButton(text("editorAddPhase"),&dialog);layout->addWidget(add);
    connect(add,&QPushButton::clicked,&dialog,[&]{append({10,SignalColor::red});});
    auto* remove=new QPushButton(text("editorRemoveLast"),&dialog);layout->addWidget(remove);
    connect(remove,&QPushButton::clicked,&dialog,[&]{if(phases->rowCount())phases->removeRow(phases->rowCount()-1);});
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&dialog);layout->addWidget(buttons);
    buttons->button(QDialogButtonBox::Ok)->setText(text("editorConfirm"));buttons->button(QDialogButtonBox::Cancel)->setText(text("editorCancel"));
    std::string created;
    validateDialog(dialog,buttons,[&]{
        auto candidate=value;candidate.offset=formNumber(offset);candidate.phases.clear();
        for(int row=0;row<phases->rowCount();++row)candidate.phases.push_back({
            formNumber(qobject_cast<QDoubleSpinBox*>(phases->cellWidget(row,0))),
            static_cast<SignalColor>(qobject_cast<QComboBox*>(phases->cellWidget(row,1))->currentData().toInt())});
        commitChange("editorEditProgram",[&](auto& d){created=putProgram(d,candidate);});
    },[this](const char* key){return text(key);});
    if(dialog.exec()==QDialog::Accepted)selectDemand(created);
}
void EditorWindow::editHead(const std::string& id,const std::optional<HeadPlacement>& placed) {
    const auto& network=history_.document().network;
    NetworkSignalHead value{id,{},0,{}};
    for(const auto& h:network.signalHeads)if(h.id==id)value=h;
    if(placed){value.lane=placed->slot.lane;value.connectorId=placed->slot.connectorId;value.position=placed->station;}
    // A new head shows the group the last one placed showed: stop lines are placed approach by
    // approach, lane after lane, so the second click needs no choice at all.
    if(id.empty() && !lastHeadSignal_.empty()) {
        const auto hash=lastHeadSignal_.rfind('#');
        if(lastHeadSignal_.rfind("c:",0)==0 && hash!=std::string::npos) {
            value.controllerId=lastHeadSignal_.substr(2,hash-2);value.groupNumber=std::stoi(lastHeadSignal_.substr(hash+1));
        } else if(lastHeadSignal_.rfind("p:",0)==0) value.programId=lastHeadSignal_.substr(2);
    }
    FormDialog dialog(this);dialog.setObjectName("editorHeadDialog");dialog.setWindowTitle(text("editorEditHead"));
    auto* form=new QFormLayout(&dialog);form->setContentsMargins(editorDesign::space3,editorDesign::space3,editorDesign::space3,editorDesign::space3);
    form->setHorizontalSpacing(editorDesign::space2);form->setVerticalSpacing(editorDesign::space1);
    auto* lane=new QComboBox(&dialog);lane->setObjectName("editorHeadLane");
    // Names first, the way the canvas shows them: "Main St · lane 2", never "link-4 / lane-9".
    // The station's upper bound is the chosen lane's own length, so a stop line cannot be typed
    // off the end of the road it holds.
    const auto addSlot=[&](const QString& label,const std::string& key,const std::string& linkId,const NetworkSignalHead& probe){
        const auto slot=headSlot(network,probe);
        lane->addItem(label,QString::fromStdString(key));
        lane->setItemData(lane->count()-1,QString::fromStdString(linkId),Qt::UserRole+1);
        lane->setItemData(lane->count()-1,slot?(slot->equation?slot->equation->arcStations.back():polylineLength(slot->geometry)):0.,Qt::UserRole+2);
    };
    for(const auto& link:network.links)for(std::size_t i=0;i<link.lanes.size();++i) {
        NetworkSignalHead probe;probe.lane={link.id,link.lanes[i].id};
        addSlot(QString::fromStdString(link.name.empty()?link.id:link.name)+" · "+text("editorLaneNumber").arg(i+1),link.lanes[i].id,link.id,probe);
    }
    for(const auto& c:network.connectors){const auto paths=connectorPaths(network,c);for(std::size_t i=0;i<paths.size();++i) {
        NetworkSignalHead probe;probe.connectorId=paths[i].id;
        addSlot(QString::fromStdString(c.name.empty()?c.id:c.name)+" · "+text("editorPathNumber").arg(i+1),paths[i].id,{},probe);
    }}
    if(!value.connectorId.empty())lane->setCurrentIndex(lane->findData(QString::fromStdString(value.connectorId)));
    if(!value.lane.laneId.empty())lane->setCurrentIndex(lane->findData(QString::fromStdString(value.lane.laneId)));
    // The signal group the head shows (M2.7b): every group of every controller, then any legacy
    // program. "New signal controller..." makes one without leaving the placement.
    auto* program=new QComboBox(&dialog);program->setObjectName("editorHeadSignal");
    const auto keyOf=[](const NetworkSignalHead& h){
        return h.controllerId.empty()?(h.programId.empty()?std::string{}:"p:"+h.programId)
                                     :"c:"+h.controllerId+"#"+std::to_string(h.groupNumber);
    };
    std::function<void(const std::string&)> fill=[&](const std::string& selected){
        program->clear();
        if(const auto& def=history_.document().definition) {
            for(const auto& c:def->signalControllers)for(const auto& g:c.groups)
                program->addItem(signalLabel(c.id,g.number,{}),QString::fromStdString("c:"+c.id+"#"+std::to_string(g.number)));
            for(const auto& p:def->signalPrograms)program->addItem(signalLabel({},0,p.id),QString::fromStdString("p:"+p.id));
        }
        const int at=program->findData(QString::fromStdString(selected));
        if(at>=0)program->setCurrentIndex(at);
    };
    fill(keyOf(value));
    auto* create=new QPushButton(text("editorNewController"),&dialog);create->setObjectName("editorHeadNewController");
    auto* signalRow=new QWidget(&dialog);auto* signalLayout=new QHBoxLayout(signalRow);signalLayout->setContentsMargins(0,0,0,0);signalLayout->setSpacing(editorDesign::space1);
    signalLayout->addWidget(program,1);signalLayout->addWidget(create);
    auto* station=number(dialog,0,10000000,value.position,"editorHeadPosition");
    const auto bound=[&]{
        const double length=lane->currentData(Qt::UserRole+2).toDouble();
        station->setMaximum(length>0?length:10000000);
    };
    bound();setFormNumber(station,value.position);
    connect(lane,qOverload<int>(&QComboBox::currentIndexChanged),&dialog,bound);
    form->addRow(text("editorColumnLane"),lane);form->addRow(text("editorSignalGroup"),signalRow);
    form->addRow(text("editorStopLine"),station);
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&dialog);form->addRow(buttons);
    buttons->button(QDialogButtonBox::Ok)->setText(text("editorConfirm"));buttons->button(QDialogButtonBox::Cancel)->setText(text("editorCancel"));
    const auto enable=[&]{buttons->button(QDialogButtonBox::Ok)->setEnabled(lane->count()>0 && program->count()>0);};
    enable();
    connect(create,&QPushButton::clicked,&dialog,[&]{
        const auto made=editController();
        if(made.empty())return;
        // The new controller's first group: the head is placed for it, so select it.
        std::string first;
        for(const auto& c:history_.document().definition->signalControllers)if(c.id==made && !c.groups.empty())
            first="c:"+made+"#"+std::to_string(c.groups.front().number);
        fill(first);enable();
    });
    validateDialog(dialog,buttons,[&]{
        auto candidate=value;
        candidate.lane={lane->currentData(Qt::UserRole+1).toString().toStdString(),lane->currentData().toString().toStdString()};
        candidate.connectorId.clear();
        if(candidate.lane.linkId.empty()){candidate.connectorId=candidate.lane.laneId;candidate.lane={};}
        candidate.position=formNumber(station);
        const double limit=lane->currentData(Qt::UserRole+2).toDouble();
        if(candidate.position>limit&&candidate.position-limit<0.0005)candidate.position=limit;
        const auto chosen=program->currentData().toString().toStdString();
        candidate.programId.clear();candidate.controllerId.clear();candidate.groupNumber=0;
        if(chosen.rfind("p:",0)==0)candidate.programId=chosen.substr(2);
        else {const auto hash=chosen.rfind('#');candidate.controllerId=chosen.substr(2,hash-2);candidate.groupNumber=std::stoi(chosen.substr(hash+1));}
        commitChange("editorEditHead",[&](auto& d){putSignalHead(d,candidate);});lastHeadSignal_=chosen;
    },[this](const char* key){return text(key);});
    dialog.exec();
}
}
