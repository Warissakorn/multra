# Vissim's branching conflict areas — manual review, 2026-11-23 (D624)

The owner asked whether MULTRA's branching areas should be adjustable like the other conflict areas,
then asked for a review of what Vissim allows. Two PTV help pages were read in full:
[Attributes of conflict areas (2025)](https://cgi.ptvgroup.com/vision-help/VISSIM_2025_ENG/Content/5_Netzbearbeiten/Konfliktflaechen_Attr.htm)
and [Using conflict areas (2024)](https://cgi.ptvgroup.com/vision-help/VISSIM_2024_ENG/Content/5_Netzbearbeiten/Konfliktflaechen_modellieren.htm).
This records what they print and what they leave unsaid; it is not a test of Vissim's behaviour.

## Status

- 2025: "Undetermined: Link A red - Link B red, for branching, so that vehicles can "see" each other.
  There is no intersection control, as vehicles simply remain in their original sequence."
- 2024: "Both red: for branching conflicts, so that vehicles can "see" each other."
- The general sentence "For each conflict area, you can select the link that has the right of way. The
  conflict area may also remain passive" is not tied to a type. **Not stated:** whether a branching area
  can be set green/red or passive.

## Attributes by type

| Attribute | What the pages say about branching |
|---|---|
| FrontGapDef (default 0.5 s) | "For the types Merge and Crossing": not branching |
| RearGapDef (default 0.5 s) | "Only for type Crossing": not branching |
| SafDistFactDef | "only for the type Merge": not branching |
| ObsAdjLns | "at merging conflicts": not branching |
| VisibLinkA / VisibLinkB | No type restriction: "Maximum distance at which an approaching vehicle can "see" the vehicles on the other link." **No default printed**; only "Avoid values < 1 m" |
| AddStopDist (0 m), AnticipRout (0 %), MinGapBlockDef (3.0 s), AvoidBlockMajor/Minor, Gaps per class | No type restriction stated; whether they act at a branching area is not stated |

Also stated: merge and branching areas "are not kept free of vehicles in the major flow"; a priority rule
is the tool for that.

## Reading for MULTRA

What the pages confirm, MULTRA already does since D623: red-red, the two branches see each other, original
order kept, no gap test. The one attribute that might apply, visibility, has no printed default, so a
MULTRA key for it would need a default from another source (rule 4). The drawn area stays the extent.
