Behaviour Lab (D165): the development ring a driving behaviour is tuned on. `lab_settings.hpp` holds
every value the window edits; `lab_network.*` builds the stadium ring through the editor's commands;
`lab_run.*` compiles it with `buildScenario`, marks its lane chains circuits, places the vehicles and
steps, retuning in place (`retuneSimulation`) when only values change and restarting otherwise. No
Qt, editor or shell (`check_architecture`); the window is `src/shell/behaviour_lab_*`. Contract:
[BEHAVIOUR_LAB](../../docs/reference/BEHAVIOUR_LAB.md). A development bed, not evidence.

`rescaleAxles` keeps valid explicit axle proportions consistent with a tuned focus length.
LabRun, the parameter panel and project Apply share this rule; strict core validation is retained.
