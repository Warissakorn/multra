# Driver-behaviour presets — not loaded into projects

The catalog loader reads only `*.json` directly in `data/driver-behaviour/`, so files here are
never part of a project's behaviours (D176). Copy one into a project's library to use it.

| File | Source | Status |
|---|---|---|
| `w74-sumo-defaults.json` | Eclipse SUMO `MSCFModel_Wiedemann.cpp` defaults at `79e42b50`, mapped in [the evidence](../../../docs/evidence/w74-preset.md) | Uncalibrated, not validated, not equivalent to SUMO or Vissim results |
