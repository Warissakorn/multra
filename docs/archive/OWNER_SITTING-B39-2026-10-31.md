# Owner sitting archive — B39 (2026-10-31)

Whole answered item, moved after J7 approval. The current sheet is
[OWNER_SITTING](../plans/OWNER_SITTING.md).

39. **J7 analysis source** (answered 2026-10-31, [D418](../decisions/RECORD.md#d418)).
   Owner chose current snapshot as recommended. Use the file's exact bytes when it still
   parses to the current document; otherwise use current Save bytes and label the result
   as an unsaved snapshot at Analyse. Saving that unchanged snapshot must give byte-identical
   CLI JSON/CSV, preserving existing CLI outputs. No automatic Save or new stored field.
