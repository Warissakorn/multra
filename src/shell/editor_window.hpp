#pragma once
#include "../project/readiness.hpp"
#include "../commands/network_commands.hpp"
#include "../commands/connector_commands.hpp"
#include "../project/diagnostics.hpp"
#include "../project/run.hpp"
#include "../project/export_marking.hpp"
#include "../eval/summary.hpp"
#include "../eval/discharge.hpp"
#include "../project/evaluation.hpp"
#include "../project/display.hpp"
#include "../runner/batch.hpp"
#include "../project/comparison_output.hpp"
#include "../commands/appearance_commands.hpp"
#include "../commands/demand_commands.hpp"
#include <array>
#include <QTimer>
#include <QElapsedTimer>
#include "../core/validate.hpp"
#include "../editor/canvas.hpp"
#include <QMainWindow>
#include <QJsonObject>
#include <atomic>
#include <map>
#include <memory>
#include <thread>
#include <filesystem>
#include <functional>
#include <vector>
#include <QKeySequence>
#include <QPointer>
#include "run_identity.hpp"

class QTreeWidget;
class QTreeWidgetItem;
class QListWidget;
class QLockFile;
class QAction;
class QComboBox;
class QVBoxLayout;
class QSpinBox;
class QDoubleSpinBox;
class QLineEdit;
class QLabel;
class QFormLayout;
class QCloseEvent;
class QTabWidget;
class QTableWidget;
class QToolBar;
namespace multra {
namespace lab { struct FocusSettings; }
class BehaviourLabWindow;
class AutomationServer;
struct MacroPanel; struct MacroRun; struct MesoPanel; struct MesoRun;
class EditorWindow : public QMainWindow {
public:
    explicit EditorWindow(const std::filesystem::path& data, const QString& language = "en", QWidget* parent = nullptr, bool deferRecovery = false);
    ~EditorWindow() override;
    void autosaveNow();
    void recoverFile(const QString&);
    QString recoveryPath() const { return recoveryFile_; }
    bool autosaveActive() const { return autosaveTimer_.isActive(); }
    const History& history() const { return history_; }
    EditorCanvas* canvas() const { return canvas_; }
    // D165: Simulation > Behaviour Lab; opens (or raises) the lab window. Null until first opened.
    void openBehaviourLab();
    BehaviourLabWindow* behaviourLab() const;
    void openFile(const QString& path); // Parse and validate before replacing the document.
    // Same, but reports a failure through this window's translated error surface instead of
    // throwing a bare code at a caller that has no locale to resolve it with.
    void openFileOrReport(const QString& path);
    void saveFile(const QString& path); // Atomic replacement; failures preserve dirty state.
    // JSON command boundary shared by the local automation server and UI integration tests.
    QJsonObject automationRequest(const QJsonObject& request);
    bool enableAutomation(const QString& endpoint, QString* error = nullptr);
    void disableAutomation();
    QString automationEndpoint() const;
protected:
    void closeEvent(QCloseEvent*) override;
    void resizeEvent(QResizeEvent*) override;
private:
    QString automationSession_; // with History::generation(), the automation documentId
    QJsonObject automationState(bool includeSelection = true);
    std::unique_ptr<AutomationServer> automationServer_;
    QString automationEndpoint_;
    bool automationStartupComplete_{};
    bool automationClosing_{}, automationCloseReady_{};
    bool automationWasEnabled_{}, automationLabWasEnabled_{};
    void beginAutomationQuit();
    void completeAutomationQuit(bool delivered);
    void newFile();
    void buildAutomationActions();
    void refreshAutomationActions();
    History history_;
    std::filesystem::path data_;
    DisplayCatalog displayCatalog_;
    std::vector<MapProvider> mapProviders_;
    void buildMapActions();
    void buildMapTranslation();
    void refreshMapActions();
    void editMapSettings();
    bool applyMapSettings(const std::optional<MapSettings>& settings);
    QJsonObject automationMap(const QString& command,const QJsonObject& params);
    void goToMapCoordinates();
    QTreeWidget* palette_{};
    std::map<int,QTreeWidgetItem*> paletteItems_;
    QListWidget* historyList_{};
    void buildHistory();
    void rotateSelection();
    void refreshHistory();
    void restoreHistory(std::uint64_t revision);
    QComboBox *objectLevel_{},*objectDisplay_{},*visibleLevel_{};
    QSpinBox *connectorFromCount_{},*connectorToCount_{},*connectorPoints_{};
    QComboBox* connectorLaneSide_{}; // M3.2.9c: laneChangeSide, one-lane differences only (D73)
    // Vissim's Lanes tab, as comma-separated text, the same shape the Link lane-width row uses.
    QLineEdit *connectorWidths_{},*connectorMarkings_{};
    QDoubleSpinBox *connectorFromPosition_{},*connectorToPosition_{};
    void buildPalette();
    void translatePalette();
    // Applies the palette view (names or icons only) and fits the docked palette to it (B17).
    void showPaletteView();
    void showCommandPalette();
    void buildAppearance(QFormLayout*);
    void refreshAppearance();
    // The Name of whatever is selected, empty when nothing is or the object carries none.
    std::string selectedName() const;
    void createLinkDialog(const std::vector<Point>&);
    void createRangeDialog(LaneReference,LaneReference,const std::vector<Point>&);
    bool deferRecovery_{};
    QString recoveryDirectory_, recoveryFile_;
    QJsonArray recoveryEntries() const;
    QString recoveryCandidate(const QString& id) const;
    void discardRecovery(const QString& id);
    std::unique_ptr<QLockFile> recoveryLock_;
    QTimer autosaveTimer_;
    std::optional<std::uint64_t> autosavedRevision_;
    void buildRecovery();
    void startAutosave();
    void clearRecovery();
    void recoverDialog(bool startup = false);
    QTableWidget *routeTable_{}, *inputTable_{}, *programTable_{}, *decisionTable_{};
    void buildDemandTables();
    void refreshDemand();
    void translateDemand();
    void editRoute(const std::string& id = {}, const std::vector<std::string>& initial = {});
    void showDemandPreview();
    void editDemandCatalog();
    // M3.3.2c (D128): driving-behaviour library dialog and Link/Connector assignment.
    void openBehaviourLibrary();
    // D165: Simulation > Behaviour Lab, and its write-back of the tuned vehicle (editor_lab.cpp).
    void buildBehaviourLab();
    QString applyLabFocus(const lab::FocusSettings&);
    QPointer<BehaviourLabWindow> lab_;
    std::string labTypeId_, labBehaviourId_; // what Apply writes to, chosen when the lab opens
    void buildBehaviourInspector(QVBoxLayout*);
    void refreshBehaviourInspector();
    std::vector<std::string> selectedRoads() const; // selected Links and Connectors, in selection order
    QComboBox* behaviourType_{};
    QLabel* effectiveBehaviour_{};
    void editInput(const std::string& id = {}, const std::string& preselectedRoute = {}, const std::string& preselectedLink = {});
    void editProgram(const std::string& id = {}); // legacy programs only (M2.7b)
    // M2.7b, src/shell/editor_signal.cpp. Returns the id committed, empty on cancel.
    std::string editController(const std::string& id = {});
    QString signalLabel(const std::string& controllerId, int groupNumber, const std::string& programId) const;
    void editDecision(const std::string& id = {}); // M2.4, src/shell/editor_decision.cpp
    // `placed` is the canvas click: the dialog opens on that lane and station.
    void editHead(const std::string& id = {}, const std::optional<HeadPlacement>& placed = {});
    void editRunSettings();
    void deleteDemand(const std::string& kind, const std::string& id);
    void selectDemand(const std::string& id);
    // Pointer authoring (M1.25): the canvas draws and gestures, the window commits. Both go
    // through the same putRoute/putInput commands the dialogs use.
    void buildRouting();
    void commitDrawnRoute(const std::vector<std::string>& segmentIds,std::optional<double> position = {}, std::optional<RouteDestination> destination = {});
    void placeInputOnLink(const std::string& linkId);
    void showDemandMenu(QPoint viewportPosition);
    void syncHighlightedRoute();
    void refreshToolHint();
    QLabel* toolHint_{};
    void buildRunControls();
    void refreshRun();
    bool prepareRun(const std::function<void(const RunSnapshot&)>& check = {});
    void runUntil(const QJsonObject& params);
    void endRunUntil(const char* reason);
    void toggleRun();
    void stepRun();
    void tickRun();
    void clearRun(const char* reason = "documentChanged");
    RunIdentity runIdentity_;
    QJsonObject automationRunState() const;
    void checkAutomationRunId(const QJsonObject&,bool required = false) const;
    void pauseRun();
    QTimer runTimer_;
    QElapsedTimer runElapsed_;
    double runCredit_{};
    std::optional<RunSnapshot> runSnapshot_;
    SimState runState_;
    // Fed every step, exactly as the retired M0 window did: the clamp count is the only signal
    // that the prototype car-following is being pushed, so it has to be visible where the run is.
    SummaryAccumulator runSummary_;
    // M2.5: per-movement delay and approach queues, fed the same states as runSummary_.
    std::optional<MovementAccumulator> runMovements_;
    // D144: queue discharge with the CLI's default spec, fed the same states; the reason it
    // could not be set up, if any, is shown instead of a table.
    std::optional<DischargeAccumulator> runDischarge_;
    std::string runDischargeError_;
    // D144: one row per SafetyClampEvent, read off the published vehicle (blank once it left).
    struct ClampRow { double time{}; std::uint64_t vehicleId{}; std::string typeId, routeId, segmentId; };
    std::vector<ClampRow> runClamps_;
    QWidget* resultsPage_{};
    QTabWidget* resultsTabs_{};
    QTableWidget *movementTable_{}, *queueTable_{}, *dischargeTable_{}, *clampTable_{};
    QLabel *resultsNote_{}, *dischargeNote_{}, *clampNote_{};
    void refreshDischarge();
    void refreshClamps();
    void buildResults();
    void translateResults();
    void refreshResults();
    bool runFinished() const;
    void observeRun();
    // M5.6 (BATCH §6), src/shell/editor_batch.cpp: Run N seeds on one worker thread over value
    // copies of the compiled document. A generation number discards whatever a cancelled or
    // replaced batch posts back, so no table ever shows fewer runs than its n.
public:
    struct BatchResult { BatchReport report; std::vector<SeedRun> runs; };
private:
    QLineEdit* batchSeeds_{};
    QLabel* batchNote_{};
    QWidget* batchView_{};
    QTableWidget *batchMovementTable_{}, *batchSectionTable_{}, *batchQueueTable_{};
    // Every live worker by generation; each is joined by its own finishBatch, or at teardown.
    struct BatchWorker { std::thread thread; std::shared_ptr<std::atomic<bool>> cancel; };
    std::map<std::uint64_t, BatchWorker> batchWorkers_;
    std::shared_ptr<std::atomic<bool>> batchCancel_;
    std::uint64_t batchGeneration_{};
    std::size_t batchDone_{}, batchTotal_{}; // batchTotal_ is non-zero while a batch runs
    std::optional<BatchResult> batch_;
    void buildBatch(QToolBar*, QVBoxLayout*);
    void translateBatch();
    void refreshBatch();
    void startBatch();
    std::optional<std::vector<std::uint32_t>> batchSeedList(); // the Seeds field, or the error shown
    void cancelBatch(); // stops a running batch or comparison and discards a finished one
    // The worker both use: `job` runs on it over value copies, reports runs done through
    // `progress` (false once cancelled) and returns what installs its result on the UI thread.
    using BatchJob = std::function<std::function<void()>(const std::function<bool(std::size_t)>& progress,
                                                       const std::function<bool()>& cancelled)>;
    void startWorker(std::size_t total, bool comparing, BatchJob job);
    void finishBatch(std::uint64_t generation, const std::function<void()>& install, const std::string& error);
    // M5.8b (BATCH §7, D149), src/shell/editor_compare.cpp: the open project as base against a
    // chosen file over the Seeds field, on the same worker; one table at a time with the batch.
public:
    struct ComparisonResult { Comparison comparison; ComparedBatch base, alternative; };
private:
    QWidget* compareView_{};
    QLabel* compareNote_{};
    QTableWidget *compareMovementTable_{}, *compareSectionTable_{}, *compareQueueTable_{};
    bool batchComparing_{}; // the running job is a comparison
    std::optional<ComparisonResult> comparison_;
    void buildCompare(QToolBar*, QVBoxLayout*);
    void translateCompare();
    void refreshCompare();
    bool batchShown() const { return batchRunning() || batch_ || comparison_; }
    // M3.2.4, src/shell/editor_priority.cpp: the Conflict areas tab, its dialogs and actions.
    QTableWidget* conflictTable_{};
    QLabel* runProtection_{};
    std::uint64_t conflictRevision_{UINT64_MAX}; // the rows are rebuilt only when this goes stale
    void buildConflicts();
    void translateConflicts();
    void refreshConflicts();
    void editConflict(const std::string& id);
    void addCrossings();
    // Shows the area in the tab and on the canvas; a waiting line or rule id names its area.
    bool selectConflict(const std::string& id);
    void showConflicts();
    void cyclePriority(const std::string& id);
    std::string selectedConflict() const;
    PriorityDefaults priorityDefaults() const; // data/priority-rules, as Run reads them
    // M3.2.4c (D68): the automatic areas, derived once per revision and only while the Conflict
    // area tool or tab is in use (3.4 ms a revision on the four-leg template, Release).
    std::vector<AutomaticConflict> automatic_;
    std::uint64_t automaticRevision_{UINT64_MAX};
    bool automaticShown() const;
    void authorConflict(const std::string& key);
    // M3.2.6c, src/shell/editor_counters.cpp: the Queue counters tab and the tool's commit.
    QTableWidget* counterTable_{};
    std::uint64_t counterRevision_{UINT64_MAX};
    void buildCounters();
    void translateCounters();
    void refreshCounters();
    void showCounters();
    void editCounter(const std::string& id);
    void addCounter(std::vector<MeasurementLine> lines);
    std::string selectedCounter() const;
    // M5.4b, src/shell/editor_sections.cpp: the Travel-time sections tab and the tool's commit.
    QTableWidget* sectionTable_{};
    std::uint64_t sectionRevision_{UINT64_MAX};
    void buildSections();
    void translateSections();
    void refreshSections();
    void showSections();
    void editSection(const std::string& id);
    std::string selectedSection() const;
    // B-6 (D223), src/shell/editor_speed_decisions.cpp: the Speed decisions tab and its dialog.
    QTableWidget* speedDecisionTable_{};
    std::uint64_t speedDecisionRevision_{UINT64_MAX};
    void buildSpeedDecisions(); void translateSpeedDecisions(); void refreshSpeedDecisions();
    void editSpeedDecision(const std::string& id, const std::string& linkId); // new on linkId when id is empty
    std::string selectedSpeedDecision() const;
    // B-6 §7 (D225): the reduced-speed areas tab, same pattern.
    QTableWidget* speedAreaTable_{};
    std::uint64_t speedAreaRevision_{UINT64_MAX};
    void buildSpeedAreas(); void translateSpeedAreas(); void refreshSpeedAreas();
    void editSpeedArea(const std::string& id, const std::string& linkId); // new on linkId when id is empty
    std::string selectedSpeedArea() const;
    // M8.6a (D161): the Zones tab -- zones, the selected zone's connectors, and their dialogs.
    QTableWidget *zoneTable_{}, *zoneConnectorTable_{};
    std::uint64_t zoneRevision_{UINT64_MAX}; std::string zoneConnectorsOf_;
    // S0.1 (NODES §6): the Nodes tab -- the drawn polygons, rename and delete.
    QTableWidget* nodeTable_{}; std::uint64_t nodeRevision_{UINT64_MAX};
    void buildNodes(); void translateNodes(); void refreshNodes(); void showNodes();
    void editCoarse(const std::string& id); // S3.6b (D311): editor_coarse.cpp
    // S4.4 (D317): Node proposals from the Macro and Meso results, editor_proposals.cpp.
    QTableWidget* nodeProposalTable_{}; QLabel* nodeProposalNote_{}; std::array<std::uint64_t, 3> nodeProposalKey_{UINT64_MAX, UINT64_MAX, UINT64_MAX};
    void buildNodeProposals(QVBoxLayout*); void refreshNodeProposals(); void translateNodeProposals();
    // S3.3b (STREET_SEGMENTS §16, D307): the StreetSegments and Cross-sections tabs and the StreetSegment tool's commit.
    QTableWidget* streetSegmentTable_{}; QComboBox* streetSegmentTemplate_{}; std::uint64_t streetSegmentRevision_{UINT64_MAX};
    QTableWidget* crossSectionTable_{}; std::uint64_t crossSectionRevision_{UINT64_MAX};
    enum class StreetSegmentStatus { noTemplate, unknownTemplate, notApplied, applied, detached };
    std::vector<CrossSection> crossSectionRows() const;
    StreetSegmentStatus streetSegmentStatus(const StreetSegment&, const std::vector<CrossSection>&) const;
    void buildStreetSegments(); void translateStreetSegments(); void refreshStreetSegments(); void showStreetSegments(); void editStreetSegment(const std::string& id);
    void findCrossingsDialog(); // S3.5 (D309)
    void seamChecksDialog(); // S3.7 (D312): editor_seams.cpp
    double seamSpeedJump_{20}; // km/h, a display choice kept for the session (STREET_SEGMENTS §7)
    std::string selectedStreetSegment() const; std::vector<std::string> selectedStreetSegmentRows() const; std::vector<std::string> selectedLinks() const;
    void buildCrossSections(); void translateCrossSections(); void refreshCrossSections(); void editCrossSection(const std::string& id);
    std::string selectedCrossSection() const;
    std::string selectedNode() const;
    void buildZones();
    void translateZones();
    void refreshZones();
    void showZones();
    void editZone(const std::string& id);
    void editZoneConnector(const std::string& id);
    void addZoneConnector(bool origin);
    std::string selectedZone() const;
    std::string selectedZoneConnector() const;
    // M8.6b (D162): the OD matrices tab, the lot dialog and the catalog's compositions for both.
    QTableWidget* odTable_{};
    std::uint64_t odRevision_{UINT64_MAX};
    void buildOd();
    void translateOd();
    void refreshOd();
    void showOd();
    void editOd(const std::string& id); // empty: a new matrix
    void editLot(const std::string& zoneId);
    std::string selectedOd() const;
    std::vector<std::pair<std::string, QString>> compositionChoices() const;
    // S1.1 (STUDY_PERIODS §8): the Study periods tab and the choices the OD dialog offers.
    QTableWidget* periodTable_{}; std::uint64_t periodRevision_{UINT64_MAX};
    void buildPeriods(); void translatePeriods(); void refreshPeriods(); void showPeriods();
    void editPeriod(const std::string& id); // empty: a new period
    std::string selectedPeriod() const;
    std::vector<std::pair<std::string, QString>> periodChoices() const;
    // S1.2 (STUDY_PERIODS §8, D269): the Modes tab -- installed modes read-only, owned ones edited.
    QTableWidget* modeTable_{}; std::uint64_t modeRevision_{UINT64_MAX};
    void buildModes(); void translateModes(); void refreshModes(); void showModes();
    void editMode(const std::string& id); // empty: a new mode
    std::string selectedMode() const;
    std::vector<std::pair<std::string, QString>> modeChoices() const;
    // S2.4 (MACRO_NETWORK §20, D277), editor_link_types.cpp: the Link types tab and the Link's type field.
    QTableWidget* linkTypeTable_{}; QComboBox* linkType_{}; QLineEdit* linkSpeed_{}; std::uint64_t linkTypeRevision_{UINT64_MAX};
    void buildLinkTypes(); void translateLinkTypes(); void refreshLinkTypes(); void editLinkType(const std::string& id);
    std::string selectedLinkType() const; void buildLinkTypeField(QFormLayout*); void refreshLinkTypeField();
    // S2.4, editor_macro.cpp: Run Macro on its own worker, in stages (§19 a); the result stays after an edit, marked stale.
    std::unique_ptr<MacroPanel> macro_;
    void buildMacro(QToolBar*, QVBoxLayout*); void refreshMacro(); void startMacro(const std::string& period, const std::string& definitionId = {}); void discardMacro();
public:
    bool macroRunning() const; const MacroRun* macroResult() const; bool macroStale() const;
    bool macroTabShown() const; // Export and Copy then write the Macro CSV
    bool mesoRunning() const; const MesoRun* mesoResult() const; bool mesoStale() const;
    bool mesoTabShown() const; // Export and Copy then write the Meso CSV
private:
    // M11.4, editor_meso.cpp: Run Meso on its own worker over the whole run; stale after an edit (D300).
    std::unique_ptr<MesoPanel> meso_;
    void buildMeso(QToolBar*, QVBoxLayout*); void refreshMeso(); void startMeso(); void discardMeso();
    // S5.5a (STUDY_AREA §8-§9, D331), editor_side_files.cpp: the Macro and Meso side files.
    void loadSideFiles(); void writeMacroSide(); void writeMesoSide(const std::vector<std::string>& watched);
    StudyAreaSources studyAreaSources() const; const std::string& projectSha() const;
    // S5.5b (STUDY_AREA §11, D332): each result's export marking and study-area flows, set where its
    // snapshot is compiled; resultsCsv() writes them.
    struct ResultMarking { ExportMarking marking; std::optional<StudyAreaProvenance> flows; };
    ResultMarking runMarking_, batchMarking_, baseMarking_, alternativeMarking_;
public:
    bool macroSideStale() const; bool mesoSideStale() const; // a side file whose projectSha256 is not the document's
private:
    QTableWidget* lotTable_{}; QLabel* lotNote_{}; // Results -> Parking lots
    QTableWidget* nodeResultTable_{}; QLabel* nodeResultNote_{}; void refreshResultNodes(); // S0.6 (NODES §15): Results -> Nodes
    void refreshComparedNodes(const QString& marker); // S0.8b (NODES §18): a comparison's Node rows
    void refreshLots();
    QLineEdit* runSeed_{};
    QComboBox* runSpeed_{};
    QLabel* runInfo_{};
public:
    const SimState& runState() const { return runState_; }
    RunSummary runSummary() const { return runSummary_.summary(); }
    std::optional<MovementReport> runReport() const {
        return runMovements_ ? std::optional(runMovements_->report(runState_)) : std::nullopt;
    }
    // The finished run's report as the CLI's CSV, replaced atomically; throws before the end.
    // With a finished batch, the batch's CSV instead (`--seeds --csv`); with a finished
    // comparison, the comparison's (`--compare --csv`).
    void exportResults(const QString& file) const;
    // The same text as tab-separated values on the clipboard (M5.6); throws when Export would.
    void copyResults() const;
    std::string singleRunCsv() const;
    std::string resultsCsv() const; // what Export writes; throws EDIT_CSV_UNFINISHED without a finished run, batch or comparison
    bool batchRunning() const { return batchTotal_ > 0; }
    const std::optional<BatchResult>& batchResult() const { return batch_; }
    // The open project (base) against `file` (alternative) over the Seeds field. Refuses, before
    // any run and leaving the results on show, a bad list, an alternative that cannot be read or
    // compiled, and evaluation periods that differ; *Compare with...* is a file dialog and this.
    void compareWithFile(const QString& file);
    const std::optional<ComparisonResult>& comparisonResult() const { return comparison_; }
private:
    QString file_;
    std::map<QString,QJsonObject> locales_;
    std::map<std::string,QAction*> actions_;
    std::map<std::string,QWidget*> texts_;
    std::vector<std::function<void()>> validationRefresh_;
    EditorCanvas* canvas_{};
    QComboBox *language_{}, *tool_{}, *side_{};
    QComboBox *connectorObject_{}, *connectorFrom_{}, *connectorTo_{};
    QTabWidget *properties_{}, *objects_{};
    // Page identity is registered once at construction; tab order is presentation only.
    std::map<std::string,QWidget*> objectPages_, resultPages_;
    void addObjectPage(QWidget* page,const std::string& key);
    void translateObjectPages();
    // Task groups (UI slice c): navigation only; the group always follows the current page.
    QComboBox* objectGroup_{};
    bool changingGroup_{};
    void buildObjectGroups();
    std::string objectGroupOf(const QWidget* page) const;
    void showObjectPage(QWidget* page);
    void revealObjectPage(QWidget* page); // also shows the Objects dock, leaving focus mode
    QTableWidget *linkTable_{}, *connectorTable_{}, *signalTable_{}, *problemTable_{};
    // Diagnostics of the current revision, plus the issues of the most recent rejected edit.
    // Both are derived views; the document stays the single source of truth.
    std::vector<Diagnostic> diagnostics_, rejected_;
    std::uint64_t tableRevision_{}, diagnosticRevision_{};
    bool syncing_{};
    QLabel* connectorHint_{};
    QSpinBox* count_{};
    QDoubleSpinBox *width_{}, *grid_{}, *split_{}, *gap_{}, *bgX_{}, *bgY_{}, *bgScale_{}, *bgAngle_{}, *bgOpacity_{};
    QLineEdit *id_{}, *name_{}, *widths_{}, *linkMarkings_{};
    QDoubleSpinBox* linkPointStation_{};
    class MessageLabel* error_{}; QLabel *coordinates_{}, *selectionInfo_{}; QWidget* canvasHost_{}; // D429; D431: the canvas dock's body
    QString text(const std::string& key) const;
    void buildInspector();
    void buildWorkspace();
    void resetWorkspace();
    void setWorkspacePreset(int preset);
    void layoutToolbars();
    void focusCanvas(bool enabled);
    QByteArray workspaceBeforeFocus_;
    bool changingWorkspace_{};
    QWidget* buildConnectorInspector();
    void refreshConnector();
    void refreshConnectorRanges();
    void buildObjectTables();
    void buildDiagnostics();
    void refreshTables(bool modelChanged);
    void refreshDiagnostics();
    void recheck(); // S4.2 (D313): the chosen level's readiness
    void fillDefaultsDialog(); // S4.3 (D316): editor_fill.cpp
    QComboBox *readinessLevel_{}, *readinessPeriod_{};
    std::vector<ReadinessRow> readiness_; bool readinessChecked_{};
    void retranslateTables();
    void jumpTo(int row);
    void deleteSelected();
    void addConnection(const LaneReference& from, const LaneReference& to);
    void connectorHint();
    void refresh(bool modelChanged = true);
    void translate();
    // The one post-edit path: History, then the run/error/refresh follow-up. Throws on refusal;
    // execute() shows the error instead, automation returns it.
    bool commitChange(const std::string& name, const std::function<void(ProjectDocument&)>& action, bool generatedCommand = false);
    bool execute(const std::string& name, const std::function<void(ProjectDocument&)>& action, bool generatedCommand = false);
    std::string lastHeadSignal_; // "c:<controller>#<group>" or "p:<program>" the last head took; the next defaults to it
    void showError(const std::exception& error);
    bool confirmDiscard();
    bool saveDialog(bool as = false);
    void importImage();
    void applyBackground();
    void measure(Point a, Point b, bool calibrate);
    QAction* action(const std::string& key, const QKeySequence& shortcut, const std::function<void()>& run);
    void label(QFormLayout* form, const std::string& key, QWidget* field);
};
}
