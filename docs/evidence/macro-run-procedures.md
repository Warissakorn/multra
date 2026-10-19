# How Visum and Aimsun organise a Macro run (D276, 2026-10-19)

The owner asked whether a Macro run has a working procedure, then asked for a study of how other
programs do it, naming PTV Visum and Aimsun Next. This note is the study. It changes no code.

**How the facts were gathered.** By web search on 2026-10-19. The help pages themselves could not be
opened from the session (their hosts did not resolve), so every statement below comes from the
search's summary of the page cited. Re-check each against the page before a value or a design rests
on it alone.

## 1. PTV Visum: the procedure sequence

- **What it is:** an ordered list of operations, run one after another ("using a procedure sequence
  makes it possible to execute several work steps successively"). Operations sit in groups.
  - For a four-step model, the generated sequence is: trip generation, distribution, mode choice,
    skim calculation, matrix aggregation, then the PrT assignment last.
  - The skim comes before the assignment.
- **Where it lives:** in the version file. Steps and groups can be copied between version files
  (2022 release notes).
- **Errors and messages:** error and calculation messages go to the log file.
  - The 2022 notes fix a fault where a multimodal assignment that raised error messages still
    counted as executed successfully, so later steps ran on a bad result.
- **Its equilibrium assignment, step by step:**
  1. Start from an all-or-nothing (best-route) assignment, or from the result of an existing static
     assignment.
  2. **Inner iteration (balancing):** shift each relation's volume from routes with high impedance
     to routes with low impedance.
  3. **Outer iteration:** search, at the current network state, for new routes with lower impedance.
     If any relation has one, balance again.
  4. **Stop:** when the convergence condition (by default the gap) holds, or at the maximum number of
     iterations.

  The objective is the integral of the impedances (Beckmann's), and demand follows Wardrop's first
  principle. Visum also offers incremental assignment, LUCE and bi-conjugate Frank–Wolfe.

Sources:
- [Setting the procedure sequence (2025)](https://cgi.ptvgroup.com/vision-help/VISUM_2025_ENG/Content/2_Verfahrensablauf_festlegen/2_4_Verfahrensablauf%20festlegen.htm)
- [Creating a procedure sequence automatically (2025)](https://cgi.ptvgroup.com/vision-help/VISUM_2025_ENG/Content/2_Nachfrage_modellieren/2_3_Verfahrensablauf%20automatisch%20erzeugen.htm)
- [The procedure of equilibrium assignment (2024)](https://cgi.ptvgroup.com/vision-help/VISUM_2024_ENG/Content/1_Benutzermodell%20IV/1_5_Ablauf%20der%20Gleichgewichtsumlegung.htm)
- [Overview of the PrT assignment procedures (2025)](https://cgi.ptvgroup.com/vision-help/VISUM_2025_ENG/Content/1_Benutzermodell%20IV/1_5_Ueberblick_ueber_die_IV-Umlegungsverfahren.htm)
- [PTV Visum 2022 release notes](https://www.umovity.com/en/ptv-visum-2022-whats-new-en.pdf?inline=)

## 2. Aimsun Next: scenario, then experiment

- **The Static Assignment Scenario** holds:
  - the traffic demand, as OD matrices per user class (for example cars and trucks);
  - an optional transit plan, whose vehicles add fixed volume;
  - "Outputs to Generate", the database the results are written to.
- **The Experiment**, under the scenario, holds the method (for example Frank and Wolfe), the
  maximum number of iterations and the relative gap. The tutorial uses 50 and 0.1. The run is
  started from the experiment's context menu.
- **Results:**
  - section and turn volumes and the convergence data are retrieved from that database
    ("Retrieve Static Traffic Assignment Results");
  - volumes are reported per user class and in total, in PCU or vehicles;
  - paths are stored only when a Path Assignment object is linked (an APA file), and only per class;
  - the Path Analysis Tool can show, for Frank–Wolfe, the paths in the order each iteration found
    them.
- **Scripting:** the Macro plug-in runs an experiment by its id (`execute`) and reads results
  (`retrieve`).

Sources:
- [Tutorial 6, Macroscopic Modelling (26.0)](https://docs.aimsun.com/next/26.0.0/Tutorials/6_Macroscopic_Modelling/6_Macroscopic_Modelling.html)
- [Static Assignment (26.0)](https://docs.aimsun.com/next/26.0.0/UsersManual/StaticScenarios.html)
- [Path Analysis Tool (26.0)](https://docs.aimsun.com/next/26.0.0/UsersManual/PathAnalysisTool.html)
- [Scripting, Macro (26.0)](https://docs.aimsun.com/next/26.0.0/UsersManual/ScriptMacro.html)

## 3. Side by side with TrafficSim (D275)

| | Visum | Aimsun Next | TrafficSim today |
|---|---|---|---|
| Where a run is defined | The procedure sequence, in the version file | A scenario (demand, outputs) and an experiment under it (method, limits) | `definition.macroSettings` (one per project) plus the period named at run time |
| Several runs kept | Several operations in one sequence | Several experiments per scenario | One; the CLI names the period |
| Steps | Generation → distribution → mode choice → skim → assignment | Assignment only, per experiment | `runMacro`: read inputs → period → graph → demand → Frank–Wolfe → outputs |
| Started | Run all, or part, of the sequence | From the experiment | CLI `--macro PERIOD`; the editor in S2.4 |
| Stop rule | Gap, or the iteration cap | Relative gap, or the iteration cap | Relative gap 10⁻⁴, or the cap 1000; `MACRO_NOT_CONVERGED` |
| Errors | Logged; a step with errors once still counted as done (fixed 2022) | — (not found) | A stage's issues refuse the run; no output is written |
| Convergence record | — (not found) | Convergence data in the output database | Final gap and iteration count only |
| Paths | — (not studied) | Optional APA file, per class | Not kept |
| Algorithm | Route-based equilibrium (inner balancing, outer route search); also bi-conjugate Frank–Wolfe | Frank–Wolfe among others | Link-based Frank–Wolfe with an exact line search (§7) |

## 4. What follows for S2.4

Both programs keep the run's definition, its limits and its convergence apart from the network. Both
stop at a gap or a cap, as ours does. Aimsun keeps a convergence record per run. Visum's 2022 fault
shows why a failing step must stop what follows. MACRO_NETWORK §19 turns this into the owner's
choice for S2.4.
