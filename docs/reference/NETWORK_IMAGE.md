# Network image PNG (M1.23.2, D367)

File → Export network image (PNG) exports the current authored drawing without requiring Run.
Choose the whole visible network or the current view area, width/height in pixels, and whether
to include backgrounds. The same action is in the command palette. Options and destination
cancellation leave the document, History, save/dirty state, selection, camera and existing
files intact. Successful writes use the existing atomic replacement without direct fallback;
the current project and recovery destinations are protected by the same guard as object CSV.

## Raster contract

- Output is an opaque white PNG, 320–8192 pixels wide and 240–8192 high, at most 32 million
  pixels. Invalid dimensions fail before allocation or file creation; encoding completes before
  replacement. Aspect ratio is preserved with white margins; no stretching or clipping to fit.
- A separate authoring canvas reuses the existing renderer, display catalog, active level filter
  and wireframe setting. No selection, edit preview, grid, HUD, running vehicles or live signal
  colours are copied. This is an authored-network illustration, not a results screenshot.
- Whole-network bounds come from the visible authoring items, excluding the aerial image, with
  a margin of 3% of the larger span (minimum 1 m). An image-only draft uses its image extent;
  an empty drawing uses a 100 m square centred at the origin. Current view uses the main
  editor viewport's axis-aligned world area. North/up orientation follows the existing canvas.
- With backgrounds enabled, the embedded image retains its authored transform and opacity.
  A visible online map uses only tiles currently loaded by the live canvas. Export neither
  downloads missing tiles nor moves the live camera to load the full network. Full-network
  coverage may therefore be partial; the options explain this limitation.
- Included online maps retain provider attribution and URL in a readable bottom footer and
  the PNG's Map attribution text metadata. The footer reduces the drawing area within the
  requested pixel dimensions. Disabling backgrounds removes both raster layers and map credit.
- PNG carries no georeferencing, world file, engineering scale claim or scientific validation.
  Pixel dimensions are literal at device-pixel ratio 1; viewport widgets/toolbars are excluded.
  The whole project and simulation/report semantics remain unchanged.

## Acceptance

| Row | Case |
|---|---|
| PI1 | Whole-network/current-area renders have chosen dimensions and visible drawing; whole-network output ignores camera zoom; live state is unchanged |
| PI2 | Asymmetric red/blue embedded image remains upright; background exclusion and blank/image-only drafts work |
| PI3 | Size-limit and protected-destination refusals preserve existing bytes; PNG reopens at a Unicode path |
| PI4 | Options cancellation and the File-menu export flow preserve the draft; labels are English/Thai |
| PI5 | A local tile fixture appears with provider credit; export causes no new requests or camera changes |

Linux offscreen and Windows native evidence are separate. Packaged Windows appearance/paths
and owner acceptance remain in OWNER_SITTING G2. M1.23's other interchange/document/rendering
work and full gate remain open.
