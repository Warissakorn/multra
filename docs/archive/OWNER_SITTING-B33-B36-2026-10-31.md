# Owner sitting archive — B33–B36 (2026-10-31)

Whole answered items, moved after the junction slices were approved.
Current questions are in [OWNER_SITTING](../plans/OWNER_SITTING.md).

33. **Signalised T templates** (asked and answered 2026-10-30, [D405](../decisions/RECORD.md#d405)).
   The shipped catalog has no signalised three-leg entry, and `nemaApproachPhases` requires four
   approaches (`NEMA_MAJOR_PAIR`), although signalised T junctions are common in Thailand.
   Which preset(s), and which NEMA numbers? Recommended and selected: one protected three-stage
   preset first (major through pair, separate protected far turn from the major, minor alone),
   rather than also shipping a permitted two-stage preset now. Keep J4's compass numbers:
   2/6 major through, 1 or 5 for its protected far turn, and 4 or 8 for the existing minor leg;
   omit phases without movements. Anchor 2/6 at zero, with the leading far phase at the preceding
   cycle's end, rather than compacting the numbers. *Owner:* “เริ่มแบบ protected สามช่วงหนึ่ง preset
   (แนะนำ)”; “คงหมายเลขตามทิศและละเฟสที่ไม่ใช้ (แนะนำ)”. Book the small **J4T** slice before U1,
   with its own acceptance rows and review. Implemented in J4T after B37/D416; no timing defaults.

34. **J5 transition construction** (asked and answered 2026-10-30, [D406](../decisions/RECORD.md#d406)). §7 had left geometry
   placement and the J5/J6 selection boundary open. Recommended choices first:
   (a) centre the entered taper on the Street segment join, half along each centreline; keep
   the road alignment, and refuse if Links are too short or the Node does not contain it,
   versus placing the whole taper upstream or downstream;
   (b) add a required template selector only for Transition rows in Find crossings in J5,
   initially unselected, versus Commands only until J6 supplies the selector;
   (c) ship two symmetric templates (kerb/kerb and far/far), with owned templates able to mix
   directions, versus shipping all four combinations. Proposed stored structure:
   `directions.forward.align` / `directions.backward.align`, forward following the end-to-start
   centrelines; `junction.legs.<id>.taper[0]` serves that incoming leg. New stored transition
   structure uses feature-selected schema 63. *Owner:* taper entirely upstream before the join;
   Transition-only selector in J5; two symmetric installed templates, with owned mixed directions.
   The owner selected (b)/(c)'s recommendations and changed (a) to upstream; see §7 for the contract.

35. **J6 contract choices** (asked and answered 2026-10-30, [D410](../decisions/RECORD.md#d410)).
   JUNCTION_TEMPLATES §5/JT7 refuses replacement after a person's detail edit, but JT11 requires
   Apply again to repair that seam. Recommended: an explicit seam action lists/replaces edited
   detail in one Undo, retaining ordinary Fill's refusal and named reference guards; alternative:
   retain refusal everywhere and revise JT11 to offer Detach only. For D402/D403's corridor
   Command, recommend discovering/regenerating the full affected shared-Link component, with
   an affected-Node list before Apply and atomic refusal if references cannot be preserved;
   alternative: require the owner to select a complete affected set. When a template change
   makes entered pocket/phase values inapplicable, recommend keeping all values in an explicit
   draft until corrected; alternative: list and clear incompatible values in the same Undo.
   No timing/length default. *Owner:* all three recommended options selected: explicit seam
   replacement with the list and reference guards; automatic complete corridor discovery;
   preserve every entered site value as an explicit draft. Do not re-ask B35.

36. **J6 site-only edit ownership** (answered 2026-10-30, [D412](../decisions/RECORD.md#d412)). `setNodeJunction` currently
   clears `Node.generated` for every recipe edit, so entering a Transition's taper in the new
   form would disable J5's generated-polygon enlargement. Recommend keeping the flag for
   site-only edits on the same template, while a new template/Detach keeps D393's clearing
   rule and polygon edits still clear it; alternative: retain clearing on every edit and
   require manual enlargement after entering taper. *Owner:* keep the current flag for
   same-template site-only edits (taper, pocket storage/taper and timing values). Another
   template, Detach and any polygon edit retain D393's clearing rule; never set a cleared flag
   back to true. Implemented in the J6 setter/forms; do not re-ask B36.
