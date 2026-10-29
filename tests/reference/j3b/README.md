# F3/F4 before J3b

These three translation units are frozen from `33a856a5` (the reviewed J3a commit), before
J3b's `generatedCommand` mode. Only a provenance comment, the symbol-isolation include and
relative include paths differ; function bodies are unchanged. `symbols.hpp` gives them
separate test-only names. They are never linked into production targets.

`junction_fill_parity_tests.cpp` runs these old F3/F4 implementations with the old/manual
History path and the current implementations with the fill path. Full document JSON must
match, including IDs, revision, controls and flags, for priority and signalised Nodes on
both driving sides; Street segment re-apply after F4 must also match. Inputs and historical
outputs assert that no Link.generated or junction recipe is present. Thus the added Link
refine pass is inactive in the reference History path.

Both implementations run with the same current value ABI, unchanged non-junction codec
and platform math. This avoids treating Linux's trigonometric rounding as a Windows golden
file. The old function bodies supply the independent reference; never update them to make
a comparison pass. These files do not replace or regenerate the TypeScript baselines.
