# Project Demand catalogs — slice 5

## Contract

A vehicle type defines one rigid vehicle's dimensions, speed range, acceleration,
deceleration, behavior reference and optional axle dimensions. A composition defines
relative positive weights over vehicle types; it never defines physical dimensions.
Names are authoring metadata. IDs are stable references, not editable labels.

Project schema 18 adds an optional `definition.compositions` array. Absence means
external composition files, preserving schema 1–17 behavior. Presence, including an
empty array, means a project-owned catalog. Existing embedded vehicle/behavior fields
keep their presence semantics. Vehicle names are stored on embedded type entries;
composition names are stored with their weights. The core receives neither metadata
nor composition authoring objects.

The catalog dialog resolves the current catalog once into local staging values.
Confirming captures all three catalogs (types, behaviors, compositions) together in
one History edit; Cancel does not change ownership, revisions or allocator state.
This makes saved edited projects independent of the installed Demand catalogs.
Legacy files without owned compositions or vehicle names retain schema 17 on save.
Owned compositions or embedded vehicle names require schema 18. Opening an old
project does not silently embed or upgrade its catalog contents.
Unedited projects continue to use installed catalogs.

All owned catalog IDs must be nonempty and unique within their catalog. Type numeric
rules and behavior references use the existing core validator. Composition entries
must refer to existing owned types, contain no duplicate type IDs, have finite positive
weights and a positive finite total. Input composition references are checked at
edit/save/load when the composition catalog is owned. Unused owned entries are checked
too. Referenced types/compositions cannot be deleted: inputs and composition members
must be updated explicitly. Changes are atomic and undoable.

The UI offers type/composition lists, add/edit/delete, immutable IDs, editable names,
engine parameters and optional axle fields. Shares are weights, not percentages;
zero in a dialog excludes a type, and at least one type must remain. Desired speeds
are displayed in m/s, dimensions in m and accelerations in m/s². A behavior selector
uses the captured behavior catalog. Behavior preset editing and new motorcycle or
articulated dynamics are outside this slice.

**Modes (S1.2, D269).** `definition.modes` follows the same presence rule: absent means
`data/modes/`, present (even empty) means the project's own. A copy (`putDemandCatalog`) takes the
modes only when an OD matrix names one, so every other copy keeps its schema; a project owning its
compositions takes the installed modes in the step that first names one. Owned modes are written
at schema 49 ([STUDY_PERIODS](STUDY_PERIODS.md) §6).

## Gates

- Old external projects retain absent composition fields and the same compiled values.
- Owned catalogs round-trip and compile with no external Demand directory.
- Invalid/duplicate weights and unknown references fail without changing History.
- Changing a type or composition changes Run and Preview from one source of truth.
- Cancel, no-op edits, Undo/Redo and save/reopen preserve catalog values and ownership.
- Type dimensions and axle sum, speed min/max and deceleration ordering use existing
  validators rather than a shell-only rule.
- Linux automated UI and native CI are distinct from owner appearance acceptance.

Time-varying compositions and type-specific routing are defined in
[DEMAND_TIME_TYPES.md](DEMAND_TIME_TYPES.md), slice 6. Reporting stays M5.

## Bundled vehicle library (D333)

The installed library contains `car`, `motorcycle`, `pickup`, `van`, `bus`,
`rigid-truck` and the existing `heavy-vehicle`. New types supply explicit dimensions,
complete axle dimensions and the same reduced prototype behaviour reference.
Their dimensions, speed ranges, acceleration and deceleration are illustrative
starting assumptions, not manufacturer measurements or calibrated class behaviour.
The motorcycle entry does not enable filtering or lane sharing by itself; the
existing behaviour and lateral settings retain their existing rules.

`urban-diverse` is an opt-in illustrative composition containing all seven types.
The original `car-only` and `urban-mixed` weights, and every physical parameter of
`car` and `heavy-vehicle`, are preserved. Existing project-owned catalogs remain
owned; they do not silently acquire newly installed types. External catalogs and
newly captured catalogs include the additions through the existing ownership rules.

Visual archetypes and colours live separately in `data/vehicle-appearance/`; see
[VEHICLE_POSE](VEHICLE_POSE.md#plan-view-appearance-d333). Changing appearance does not
change the dimensions used by traffic following, turning or queue measurements.
