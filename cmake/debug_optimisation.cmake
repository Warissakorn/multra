# D621: a GCC or Clang Debug build compiles with -Og, keeping -g. Measured 2026-11-22 (4 cores,
# GCC 13.3): the full desktop suite took 331 s at -O0 (313-336, 4 runs) and 138-143 s at -Og, with
# 367/367 passing and `multra-cli 42` byte-identical. Most of the -O0 time was unoptimised STL
# and member-wise moves inside simulation runs. Results cannot shift with the level: the options
# target sets -fno-fast-math and -ffp-contract=off. MSVC, the sanitizer and release presets are
# unchanged. Configure with -DMULTRA_DEBUG_OG=OFF to step through code at -O0.
option(MULTRA_DEBUG_OG "Compile GCC/Clang Debug builds with -Og (D621)" ON)
if(MULTRA_DEBUG_OG AND NOT MSVC)
    # Directory-wide, so every target defined after this include gets it, as the measured build did.
    add_compile_options($<$<AND:$<CONFIG:Debug>,$<COMPILE_LANGUAGE:C,CXX>>:-Og>)
endif()
