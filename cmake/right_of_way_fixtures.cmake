# Shipped right-of-way projects, each regenerated from its one builder in tools/ (data/projects/README.md).
# Regenerates the M3.2.7 T-junction from its one source, tools/t_junction_network.hpp.
add_executable(multra-t-junction-fixture tools/t_junction_fixture.cpp)
target_link_libraries(multra-t-junction-fixture PRIVATE multra_commands multra_options)
target_precompile_headers(multra-t-junction-fixture REUSE_FROM multra-four-leg-fixture)
# Regenerates the PRIORITY_RULES lab (D614-D616) from tools/priority_rule_network.hpp.
add_executable(multra-priority-rule-fixture tools/priority_rule_fixture.cpp)
target_link_libraries(multra-priority-rule-fixture PRIVATE multra_commands multra_options)
target_precompile_headers(multra-priority-rule-fixture REUSE_FROM multra-four-leg-fixture)
# Regenerates the YIELD_IMPATIENCE lab (D618) from tools/yield_impatience_network.hpp.
add_executable(multra-yield-impatience-fixture tools/yield_impatience_fixture.cpp)
target_link_libraries(multra-yield-impatience-fixture PRIVATE multra_commands multra_options)
target_precompile_headers(multra-yield-impatience-fixture REUSE_FROM multra-four-leg-fixture)
# Regenerates the line-stop lab (D671) from tools/line_stop_lab_network.hpp.
add_executable(multra-line-stop-lab-fixture tools/line_stop_lab_fixture.cpp)
target_link_libraries(multra-line-stop-lab-fixture PRIVATE multra_commands multra_options)
target_precompile_headers(multra-line-stop-lab-fixture REUSE_FROM multra-four-leg-fixture)
