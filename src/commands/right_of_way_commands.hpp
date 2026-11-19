#pragma once
// M3.2.2: edits to the authored right-of-way controls. Each is a plain document change; the caller
// runs it inside History::execute, which validates the whole document and makes it one undoable
// step. A failed edit therefore changes nothing (docs/reference/M3_CONTRACT.md §6).
#include "../project/document.hpp"
#include "../model/network/right_of_way.hpp"
#include <optional>

namespace multra {
// Empty id allocates; the same id replaces. Structural checks run when the edit commits.
std::string putWaitingLine(ProjectDocument&, WaitingLine);
std::string putConflictArea(ProjectDocument&, ConflictArea);
std::string putPriorityRule(ProjectDocument&, AuthoredPriorityRule);
// M3.2.5. Empty id allocates ("stop-N"); the same id replaces.
std::string putStopControl(ProjectDocument&, StopControl);
void deleteStopControl(ProjectDocument&, const std::string& id);
// M3.2.6b. Empty id allocates ("counter-N"); the same id replaces.
std::string putQueueCounter(ProjectDocument&, AuthoredQueueCounter);
void deleteQueueCounter(ProjectDocument&, const std::string& id);
// M5.4 (D133). Empty id allocates ("section-N"); the same id replaces.
std::string putTravelTimeSection(ProjectDocument&, TravelTimeSection);
void deleteTravelTimeSection(ProjectDocument&, const std::string& id);
// Refused (EDIT_REFERENCED) while a conflict side still waits at it or a Stop/Yield control names it;
// a queue counter measuring there loses that line.
void deleteWaitingLine(ProjectDocument&, const std::string& id);
// Removes the area's rule with it: a rule means nothing without its area.
void deleteConflictArea(ProjectDocument&, const std::string& id);
// Leaves the area -- and so its merge group -- a Run-blocked draft. Deleting a rule never hands
// the group back to the drawing-order fallback behind the author's back (§3, policy 4).
void deletePriorityRule(ProjectDocument&, const std::string& id);

// Takes one merge over from the drawing-order fallback: one area, waiting line and rule per pair
// of its incoming paths, prefilled with the fallback's order and `defaults`' two numbers, so
// the result behaves exactly as the fallback did until the author changes it. `section` is a
// MergeGroup::section. Throws EDIT_NO_MERGE for a section no merge arrives on, EDIT_ALREADY_EXPLICIT
// when the group is already authored, EDIT_NO_PRIORITY_DEFAULTS when the numbers are unusable.
// Returns the area ids created.
std::vector<std::string> takeOverMerge(ProjectDocument&, const std::string& section, const PriorityDefaults&);
// The separate, named action that hands a merge back: removes every area covering two of its
// incoming paths, their rules, and the waiting lines nothing else uses.
void restoreAutomaticPriority(ProjectDocument&, const std::string& section);
// M3.2.12 (STOP_LINES §4, SL5, D594): every stored waiting line the author did not make their own
// goes, and the sides that named it wait at their derived lines. Kept: a line a Stop/Yield or a
// queue counter names, and a dragged one -- more than 0.01 m from where the editor put it, the
// derived position of the first area on its path that names it (or that area's entry, the D59
// merge strip's line). Returns how many lines went.
std::size_t deriveStopLines(ProjectDocument&);
// The inverse: every derived side gets a stored line where it waits now, so the drawing can be
// written for a reader older than schema 79. Runs as before (SL6). Returns how many lines it added.
std::size_t storeDerivedLines(ProjectDocument&);
// M3.2.12 slice 3 (STOP_LINES §2, §5, SL4). The areas a stop line at `at` would cover: every
// decided area whose yielding side waits at its derived line, on the line's own path at or past
// it, or -- for a line on a Link lane -- on a Connector leaving that lane at or past it. An area
// that already names a line keeps it, so a second line upstream covers nothing the first does.
std::vector<std::string> stopLineCoverage(const Network&, const ControlPoint& at);
// Places the line and names it on those areas; refused (EDIT_NO_CONFLICT_DOWNSTREAM) when there are none.
std::string placeStopLine(ProjectDocument&, const ControlPoint& at);
// The line goes with its Stop/Yield; the areas that named it wait at their derived lines again.
void removeStopLine(ProjectDocument&, const std::string& lineId);

// M3.2.4, the editor's gestures (src/commands/conflict_authoring.cpp). Each is one document change
// the caller runs inside History::execute.
// One crossing area per lane pair of two objects (Links or Connectors) whose lane surfaces really
// overlap (surfaceOverlap), extents exactly that overlap, one waiting line per lane 1 m short of the
// first area that lane meets and shared by all its areas (D63), and a rule with `defaults`' two numbers. `yielding` names the object that gives way.
// Returns the area ids, in lane order. Throws EDIT_SAME_OBJECT, EDIT_UNKNOWN_OBJECT, EDIT_NO_CROSSING
// (no lane pair overlaps) or EDIT_NO_PRIORITY_DEFAULTS.
std::vector<std::string> addCrossingAreas(ProjectDocument&, const std::string& first, const std::string& second,
                                          const std::string& yielding, const PriorityDefaults&);
// The area's name, priority and its rule's two numbers in one step; creates the rule if missing.
void setConflictControl(ProjectDocument&, const std::string& areaId, const std::string& name,
                        ConflictPriority, double gapTime, double headway);
// "Take over merge" on a Connector: every merge its paths arrive on that is still automatic.
// Throws EDIT_NO_MERGE when there is none left to take over.
std::vector<std::string> takeOverMergesOf(ProjectDocument&, const std::string& connectorId, const PriorityDefaults&);
// "Restore automatic priority" from one of the group's areas. Throws EDIT_NO_MERGE for a crossing.
void restoreAutomaticPriorityOf(ProjectDocument&, const std::string& areaId);
// M3.2.4b, the canvas gestures. The next priority in firstYields -> secondYields -> undetermined
// -> firstYields, keeping the name and the rule's numbers (`defaults` when the area has none);
// returns the new priority. There is no "passive" state: an unauthored crossing is not an area.
ConflictPriority cycleConflictPriority(ProjectDocument&, const std::string& areaId, const PriorityDefaults&);
// Slides a waiting line along its own path. A station past the area's entry is kept, not
// refused: the resolver reports it (CONFLICT_WAITING_LINE_AFTER_ENTRY) and Run refuses it.
void moveWaitingLine(ProjectDocument&, const std::string& lineId, double station);
// M3.2.5, the editor's control gesture: what a driver must do at the line where this area gives
// way -- Stop, Yield, or nothing (std::nullopt). The control belongs to the line, so it covers
// every decided area that gives way at that line (M3.2.5b: a lane's crossing areas share one line,
// D63). Throws EDIT_UNKNOWN_OBJECT, or EDIT_UNDETERMINED_PRIORITY while no side gives way.
void setAreaControl(ProjectDocument&, const std::string& areaId, std::optional<StopMode>);
// M3.2.4c (D68): the Conflict area tool's first click on an automatic area (automaticConflicts).
// A passive crossing becomes one authored area for that lane pair -- a turning Connector giving way
// to a Link, else the second side -- with a rule from `defaults` and each lane's shared waiting
// line (D63). A derived merge is taken over whole (takeOverMerge), unchanged in behaviour. Returns
// the area now standing where the click was. Deleting it (removeConflictArea) makes the pair
// automatic again. Throws EDIT_NO_CROSSING / EDIT_NO_MERGE when the pair is no longer automatic.
std::string authorAutomaticConflict(ProjectDocument&, const AutomaticConflict&, const PriorityDefaults&);
namespace detail {
// The same for a loop that took `current` from automaticConflicts(d.network) and since authored
// only right of way: no geometry moved, so `current` is still what that list says, and skipping
// its rebuild per pair is what makes generating a Node linear in its pairs. A crossing is not
// checked to be passive still: a caller acting on a click looks the key up in a fresh list first.
std::string authorCurrentConflict(ProjectDocument&, const AutomaticConflict& current, const PriorityDefaults&);
}
// Deletes an area with its rule and the waiting lines no other area uses.
void removeConflictArea(ProjectDocument&, const std::string& areaId);
// Group gestures, executed once inside History. Existing lane-level commands remain available
// for explicit per-pair edits. Priorities here are relative to the selected authored member.
std::string authorConflictGroup(ProjectDocument&, const std::string& key, const PriorityDefaults&);
void setConflictGroupControl(ProjectDocument&, const std::string& id, const std::string& name,
                             ConflictPriority, double gapTime, double headway);
void cycleConflictGroupPriority(ProjectDocument&, const std::string& id, const PriorityDefaults&);
void setConflictGroupStopControl(ProjectDocument&, const std::string& id, std::optional<StopMode>);
void removeConflictGroup(ProjectDocument&, const std::string& id);

}
