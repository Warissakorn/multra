# Desktop automation

MULTRA can accept structured commands while its native editor window remains visible.
An AI agent or another local program uses `multra-control` to send JSON requests to the
running `multra-desktop` process. Editing, selection and simulation playback happen in
that same window. No Python, Node.js, web server or mouse-coordinate automation is required.

This feature requires a build containing `multra-control`; an older downloaded executable
does not gain these commands without rebuilding or downloading an updated package. Build
the desktop preset, or use an updated Windows x64 package. The headless preset continues
to provide `multra-cli` for batch simulation without a window.

## Start and connect

Start the editor with an explicit local endpoint:

```powershell
.\bin\multra-desktop.exe --automation multra-ai --language th
```

The endpoint is opt-in. Windows uses a local named pipe; Unix uses a local socket.
Access is restricted to the local user through Qt's `UserAccessOption`. There is no
network listener. Another account must not be able to take the name first and receive the
requests, so both ends resolve it the same way (`automationServerName`):

- **Unix:** the socket is `multra-<hash>` inside the user's runtime directory
  (`$XDG_RUNTIME_DIR`, or Qt's owner-only fallback), never the shared temporary directory.
- **Windows:** any account can create a pipe of a given name, so `multra-control` asks the pipe
  for its server process and refuses with `UNTRUSTED_SERVER` unless that process runs as the
  same Windows user. Only the same-user path is tested; a second account is needed to see the
  refusal.

 Endpoint names contain 1–64 ASCII letters, digits, dots, underscores or hyphens,
starting with a letter or digit. Use a different endpoint for each editor process. Starting another
server on an occupied endpoint fails rather than taking over the existing session.

Create `status.json` as UTF-8 JSON:

```json
{"id":"inspect-1","command":"status"}
```

Then send it from PowerShell:

```powershell
.\bin\multra-control.exe --endpoint multra-ai --file .\status.json
```

Using `--file` avoids differences in native JSON argument quoting between PowerShell
versions. `--request '<JSON>'` and `--request -` (read standard input) are also available.
`--timeout <milliseconds>` bounds the client's wait; the default is 10000 ms.
The client prints one JSON response to standard output and exits nonzero on a failed
command or transport error. A timeout does not establish whether a submitted command
executed: inspect the current state before retrying a mutation. Request IDs correlate
replies; the server does not deduplicate requests or guarantee exactly-once execution.

## Request and response

Each request is one UTF-8 JSON object followed by a newline. A connection may carry
several requests; the client executable sends one request per invocation. Request lines
are limited to 1 MiB. The desktop processes commands on its GUI thread.

```json
{
  "id": "step-1",
  "command": "simulation.step",
  "documentId": "copy the value from response.state.documentId",
  "expectedRevision": "copy the value from response.state.revision",
  "params": {"steps": 10}
}
```

Replies preserve `id`, include `ok`, and provide either `result` or a structured
`error` with `code` and `message`. Editor command replies include `state` describing
the document and visible run. Validation failures also expose their issues.

`state.documentId` identifies the current document session; `state.revision` is a
decimal string. Keep both as strings. Every command except `capabilities`, `status`,
`document.get`, `results.get`, `recovery.list`, `map.get`, `map.status` and `connector.suggest` requires the two guard fields from the latest state.
This prevents a delayed AI edit from overwriting changes made by the user. A rejected
guard requires a fresh inspection and a decision about whether the original edit
still applies. Do not automatically retry it against the new revision.

State includes `dirty`, `file`, `selection`, `canUndo`, `canRedo`, `busy`, and `run` with `running`, `prepared`, `finished`,
`tick`, `time` and `seed`. A document revision guards authored content, rather than
elapsed simulation time. Guarded commands are refused with `BUSY` while a modal editor dialog or a menu is open, or
a mouse button is held (a drag would commit over the patched document); finish in the window
and inspect again. `simulation.reset` stores its seed as the canonical decimal (`"007"` → `"7"`).

## Commands

| Command | Parameters | Effect |
|---|---|---|
| `capabilities` | None | Discover supported commands and protocol details |
| `status` | None | Read current document and run state |
| `document.get` | Optional `pointer`: JSON Pointer string | Read `result.document`, or only the pointed `result.value` |
| `document.patch` | `patch`: RFC 6902 operation array | Validate and commit one atomic, undoable document edit |
| `connector.suggest` | `from`, `to`; optional lane counts | Preview a UI-generated Connector and an explicit patch without editing |
| `map.get` | None | Read settings and installed providers, with tile state |
| `map.status` | None | Read current viewport tile counts and readiness |
| `map.set` | `longitude`, `latitude`; optional provider/appearance/anchor fields | Replace map settings as one Undo step |
| `map.remove` | None | Remove map settings as one Undo step |
| `map.retry` | None | Retry failed visible tiles without changing History |
| `recovery.list` | None | List recovery IDs and advisory availability without opening a dialog |
| `recovery.restore` | `id`; `discardChanges: true` when dirty | Restore an inactive draft as untitled/dirty |
| `recovery.discard` | `id`, `confirm: true` | Delete one inactive draft under its lock |
| `file.new` | Optional `discardChanges: true` when dirty | Start an empty document in the same window |
| `app.quit` | Optional `discardChanges: true` when dirty | Accept intentional program quit; caller separately verifies exit |
| `file.open` | `path`; `discardChanges: true` when dirty | Open a project or supported authoring scenario in the window |
| `file.save` | `path` | Atomically save the current project to an explicit path |
| `history.undo` | None | Undo the previous document edit and clear its run |
| `history.redo` | None | Redo the next document edit and clear its run |
| `simulation.start` | None | Start or resume visible playback; repeated start is idempotent |
| `simulation.pause` | None | Pause visible playback |
| `simulation.step` | `steps`: integer 1–200 | Pause and advance a bounded number of fixed simulation steps |
| `simulation.reset` | Optional `seed`: unsigned 32-bit decimal string | Reset the visible run, optionally selecting a seed |
| `view.fit` | None | Fit the network in the canvas |
| `view.select` | `ids`: object-ID array | Select objects in the visible canvas |
| `view.screenshot` | `path` | Save a PNG image of the editor window |
| `results.get` | None | Read available single-run results with validation marking |
| `results.export` | `path` | Export finished results using the editor's existing CSV semantics |

Paths are interpreted on the computer running the desktop. Absolute paths make callers
independent of its working directory. Requests never invoke a file picker, confirmation
dialog or arbitrary application action. Opening a dirty document requires an explicit
`discardChanges: true`; save first when those edits must be kept.

`document.patch` operates on the JSON returned by `document.get`, using RFC 6902
`add`, `remove`, `replace`, `move`, `copy` and `test` operations. The whole patch must
validate before publication. The editor's History owns the revision; patches cannot
change `revision` or the `format` marker. Embedded background image bytes cannot be
replaced through this interface. Use ordinary project files to load another image.
The document's feature schema still applies: adding a field introduced in a newer
schema must use a schema version that accepts that field, up to the supported version
55. `schemaVersion` can be patched but must remain present; removing it would select
the legacy scenario decoder. Saving selects the schema required by the document's
features. Successful edits refresh
the same canvas and invalidate the previous run, as ordinary editor edits do.

An agent should inspect the document, construct the smallest applicable patch and
include a `test` operation when an edit assumes a particular old value. Native Undo
can reverse a successful patch. Failed parsing, patch application or validation leaves
the document and history intact. Empty or unrunnable drafts may be saved; simulation
still requires the existing run-readiness checks.

For example, after reading `/network/links/0/name`, use a patch like the following
inside the guarded request's `params` object (substitute the actual old name):

```json
{
  "patch": [
    {"op":"test","path":"/network/links/0/name","value":"Old name"},
    {"op":"replace","path":"/network/links/0/name","value":"East approach"}
  ]
}
```

Reading a JSON Pointer reduces response size when only one object or field is needed.
Guard mismatches return `STALE_DOCUMENT`; a protected metadata/image change returns
`PROTECTED_FIELD`. Invalid document references and values return `VALIDATION_FAILED`
with `error.issues` when the underlying validator supplies them.

`simulation.start` schedules the existing timer and returns promptly. Poll `status`
to follow progress, or use bounded `simulation.step` calls for reproducible inspection.
Each step feeds the normal event accumulators; each request renders the final vehicle frame.
Unrunnable drafts return `RUN_NOT_READY` with `error.diagnostics`. The
engine's fixed time step and seeded behavior are unchanged. `results.export` requires
exportable results and follows the currently selected Results subtab, as the ordinary
Export action does. On the Macro or Meso subtab it exports that tab's existing result;
elsewhere it exports the finished batch, comparison or single Micro run. A paused
partial Micro run is insufficient. `results.get` reads single-run Micro output
independently of that selected tab.
Reset clears the current run without preparing another one; the next Start or Step
compiles the current document. `results.get` returns `result.summary` and, when a
single run is prepared, `result.movement`, with `validated: false`.

## Windows PowerShell example

Run the following from the extracted package directory. It launches the visible editor,
waits up to ten seconds for the endpoint, opens the bundled crossing scenario, advances
ten steps and saves a screenshot. The window remains open for the user.

```powershell
# Decode UTF-8 JSON correctly, including Thai names, in Windows PowerShell 5.1.
[Console]::OutputEncoding = New-Object Text.UTF8Encoding($false)
$bin = (Resolve-Path .\bin).Path
$endpoint = 'multra-ai-' + [guid]::NewGuid().ToString('N')
$control = Join-Path $bin 'multra-control.exe'
$requestFile = [IO.Path]::GetTempFileName()
$utf8 = New-Object Text.UTF8Encoding($false)
$script:multraState = $null

function Send-Multra([string]$command, [hashtable]$params = @{}, [switch]$Guarded) {
    $request = @{ id = [guid]::NewGuid().ToString(); command = $command; params = $params }
    if ($Guarded) {
        $request.documentId = $script:multraState.documentId
        $request.expectedRevision = $script:multraState.revision
    }
    $json = ConvertTo-Json -InputObject $request -Depth 100 -Compress
    [IO.File]::WriteAllText($requestFile, $json, $utf8)
    $raw = & $control --endpoint $endpoint --timeout 1000 --file $requestFile
    if ($LASTEXITCODE -ne 0) { throw "MULTRA command failed: $raw" }
    $reply = $raw | ConvertFrom-Json
    if (-not $reply.ok) { throw ($reply.error | ConvertTo-Json -Compress) }
    $script:multraState = $reply.state
    return $reply
}

try {
    $desktop = Start-Process -FilePath (Join-Path $bin 'multra-desktop.exe') `
        -ArgumentList '--automation', $endpoint, '--language', 'th' -PassThru
    $deadline = [DateTime]::UtcNow.AddSeconds(10)
    $connected = $false
    do {
        try { $null = Send-Multra 'status'; $connected = $true }
        catch {
            if ($desktop.HasExited -or [DateTime]::UtcNow -ge $deadline) { throw }
            Start-Sleep -Milliseconds 100
        }
    } until ($connected)

    $scenario = Join-Path $bin 'data\scenarios\crossing.json'
    $null = Send-Multra 'file.open' @{ path = $scenario } -Guarded
    $null = Send-Multra 'simulation.step' @{ steps = 10 } -Guarded
    $null = Send-Multra 'view.fit' -Guarded
    $png = Join-Path (Get-Location).Path 'multra-visible-run.png'
    $null = Send-Multra 'view.screenshot' @{ path = $png } -Guarded
    Send-Multra 'results.get' | ConvertTo-Json -Depth 30

    # Continue animated playback, then inspect or pause from another command:
    # $null = Send-Multra 'simulation.start' -Guarded
    # $null = Send-Multra 'simulation.pause' -Guarded
} finally {
    Remove-Item -LiteralPath $requestFile -ErrorAction SilentlyContinue
}
```

On Linux the same protocol is available through `./bin/multra-desktop --automation
multra-ai` and `./bin/multra-control --endpoint multra-ai --file request.json`.

## Verification and limits

Automation tests exercise live Qt widgets, document validation and history, stale
guards, simulation state and local IPC. Local Linux offscreen tests do not establish
Windows runtime, high-DPI, packaged dependencies or real-window acceptance. Run the
PowerShell workflow on the updated Windows package and inspect that changes, selection
and vehicle playback appear in the visible window.

The interface does not add a second model or simulation engine. Traffic results remain
**not yet validated**; mean completed-trip delay is not HCM control delay. See
[SIMULATION.md](SIMULATION.md) and [NETWORK_EDITOR.md](NETWORK_EDITOR.md).

## Live automation lifecycle (D344)

The existing editor can enable or disable local automation from Help → AI control.
Enabling asks for an endpoint and reports it in the menu and status bar. A failed bind
keeps the editor, document, Undo, selection and run intact. `--automation` remains supported;
its startup bind failure still exits with code 2 for existing scripts.

No server is allocated until enable is requested. Disabling closes the listener and client
connections, releasing the endpoint. It does not cancel completed commands, stop simulation,
change the document or disable recovery. An incomplete request is discarded; a submitted
command may have executed even if its reply was lost. Do not retry mutations blindly.
There is no automation polling timer, heartbeat or background JSON generation.

Responses add a window-lifetime `state.instanceId`, `connectionReady` (listener active),
`startupComplete` (startup recovery handling returned), and `busyReason` (null, modal,
menu or pointer). Existing document/revision guards, busy boolean and protocol v1 remain.
A fresh client uses status/capabilities and the latest response state; no persistent client
session is required. `status` accepts `includeSelection: false` in params to omit the
selection array and its copy; `selectionCount` remains. Other callers retain their existing
selection response. Readiness does not imply a runnable document or absence of busy UI.

Acceptance: enable after editing, refuse an occupied endpoint without losing work, disable
with a connected client and pending bytes, re-enable the same endpoint, reconnect while a
run advances, and reject stale document guards. No AI timer is added. Linux offscreen
checks do not establish Windows focus, named-pipe or visible-desktop acceptance.

## New documents and intentional quit (D345)

`file.new` and `app.quit` require the existing document/revision guards and no modal,
menu or pointer gesture. Both refuse dirty work with `UNSAVED_CHANGES` unless params
contains the boolean `discardChanges: true`; a failed save never grants permission to
close or discard. `file.new` uses the same reset path as File → New, clears the old run,
Undo, selection and this window's recovery, and starts another document in the same instance.

`app.quit` is an explicit end-of-program operation, not the end of an AI task or connection.
It replies `result.quitAccepted: true`, `state.closing: true`. Closing rejects further
stateful commands with `CLOSING` and freezes editing while the reply drains. Only after
all reply bytes have left Qt's write buffer does the editor close without another save
prompt and remove its own recovery. This is transport handoff, not proof the caller read
the reply. The caller must separately observe process exit; timeout must never trigger a
kill or blind retry. If reply writing fails or cannot drain within 5 seconds, closing is
cancelled, editing is restored and recovery is retained. Read-only status remains available
while closing. No timer is installed until an intentional quit reply needs draining.

## Deferred recovery management (D346)

`--recovery ask|defer` defaults to ask, including with automation. Defer skips only the
startup prompt: autosave and File → Recover remain enabled, and no draft is deleted.
`state.recoveryPolicy` reports the choice; status performs no recovery-directory scan.

`recovery.list` is unguarded and returns `result.entries`: basename `id`, `modifiedAt`,
`bytes` and `availability` (available, current, locked or unavailable). It reads metadata,
not project contents. Availability is advisory; restore/discard recheck files and locks.
Listing is explicit, with no polling or background scan. Symlinks are not offered.

`recovery.restore {id, discardChanges?}` is guarded and requires explicit boolean discard
when the current document is dirty. It validates the selected draft before replacing the
current document, adopts its lock and opens untitled/dirty with a new documentId, so Save
must name a destination. `recovery.discard {id, confirm: true}` is guarded, requires explicit
confirmation and deletes only the named inactive draft while holding its lock. Both reject
this window's current draft, other live owners, missing files, paths and symlinks; IDs are
basenames from list, never arbitrary paths. A listed draft may become busy before use.
No command removes another editor's active draft or bulk-deletes pending recovery.

## Map commands and viewport readiness (D347)

`map.get` returns persisted settings (null when absent), installed provider IDs/names and
current tile state; `map.status` returns tile state only. Both are unguarded reads and do
not fetch tiles or rebuild the document. `map.set` replaces settings using required finite
`longitude` [-180,180] and `latitude` [-80,84]; optional `providerId` defaults to the first
installed provider, `opacity` to 0.7, `anchorWorldX/Y` to 0 and `rotationDegrees` to 0.
World anchors are within ±10,000,000 m, rotation ±180 degrees, opacity [0,1]. UTM CRS is
computed by the existing rule; clients do not patch schemaVersion or author a projection.
Unknown keys/type/range failures include error.path and error.expected without changing work.

Guarded map.set/remove use the existing map command, validation, History and run invalidation.
A changed set shows the layer and centers on a changed geographic anchor as the settings UI
does; no-op set keeps the camera and revision. Remove is idempotent. `map.retry` is guarded
view state, reusing existing failed-tile retry and Retry-After behavior without an Undo entry.

Tile state reports status (disabled/loading/ready/networkError/outsideStudy/unavailable/
renderLimit), visible, viewportPending, wanted, loaded, pending, failed and ready. Counts
refer to the current computed viewport; while viewportPending they may describe the previous
view. ready requires a settled viewport, visible layer, status ready, at least one wanted
tile and all wanted tiles loaded without failures. Pending includes retry waits; networkError
can coexist with pending retries and is not necessarily terminal. Readiness means tile data
is available to paint, not that a screenshot has already been captured. Read again after
camera changes; no fixed sleep duration or successful external OSM access is assumed.

## Connector proposals and attachment diagnostics (D348)

`connector.suggest` is an unguarded read. Required from/to each contain linkId, laneId
and optional station in metres along the Link reference geometry; omitted station means
source end/target start. Optional fromLaneCount/toLaneCount are integers 1–12, bounded by
the available contiguous lanes. The helper runs the same addConnectorRange/default-curve
command as the UI on a document copy and validates the result. The live document, ID
allocator, History, selection and run are unchanged. No snapping or auto-apply occurs.

The result includes connector, patch and rangeAdjusted plus requested/actual lane counts.
D75 range narrowing is reported explicitly. The returned patch tests/replaces nextId and
adds the connector; apply it only with document.patch and the document/revision state from
the suggestion response. IDs are provisional, not reservations. Proposals refuse allocator values outside the exact
JSON integer range rather than return a lossy nextId patch. Conflicting edits require
another suggestion; do not update stale guards and blindly apply an old proposal.

Rejected document.patch attachment issues keep VALIDATION_FAILED and existing code/path.
Where the rejected candidate can be decoded, DISCONNECTED_GEOMETRY additionally reports
objectId, expected/actual coordinates, distance, tolerance and geometryPointer. Measurements
come from the same attachment rule/tolerance used by network validation and refer to the
rejected candidate, never the current live geometry. Other issues retain their existing
shape; extra details are omitted if the candidate cannot be decoded safely.

## Single-run identity and explicit export (D349)

A successful preparation of the existing GUI single simulation assigns `state.run.runId`.
It stays the same across Step, Start, Pause, Resume, reconnect and completion; failed
preparation assigns no ID. State adds status (idle/prepared/running/paused/completed),
sourceDocumentId and sourceRevision strings, plus the latest invalidated lastRunId and
lastEndReason (reset/seedChanged/documentChanged/replacedByBatch/replacedByComparison).
Only one current single run is retained; this is not a job registry or historical result store.
Edits, Undo/Redo, New/Open/recovery, seed changes, Reset and batch/comparison replacement
invalidate the ID through the existing clearRun path. A new prepared run gets another ID.

Simulation commands and results.get accept optional runId. When supplied it must name the
current prepared run or the command returns STALE_RUN before doing work. Omitting it keeps
existing behavior, including creating a run through Step/Start. results.get adds run metadata
and keeps its existing partial summary/movement data and not-yet-validated marker.

`results.export {path, report: "movements", runId}` explicitly exports the finished single
run through the existing movement CSV formatter and atomic file writer, independent of
selected tab, Macro/Meso panels, batch or comparison. runId alone also selects this explicit
single-run path. A missing/invalid ID, mismatched run, unsupported report or unfinished run
is refused before touching the destination. The reply names path/report/run/source identity
and validated:false. CSV bytes/provenance markings stay identical to the GUI single export.

Requests with neither report nor runId preserve legacy tab-based results.export behavior.
Explicit identities/exports for batch, comparison, Macro and Meso remain out of this slice;
unsupported explicit report names are rejected, never routed to a different result silently.
No additional simulation loop, worker, timer or persistent run log is introduced.
