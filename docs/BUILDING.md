# Building MULTRA

## Requirements

- C++20 compiler: GCC 12+, recent Clang, or Visual Studio 2022 MSVC.
- CMake 3.24+ and Ninja for the checked-in presets.
- nlohmann/json 3.11+ (header-only; provided by your package manager).
- Qt 6.4+ Widgets for the desktop; Qt Test for the desktop smoke test.

The headless engine/CLI does not need Qt. The application, CLI, tests and repository
checks are C++. CMake, JSON, Markdown and workflow YAML remain their respective data,
build and documentation formats. There is no npm or Python step in the normal build.

## Windows — Visual Studio 2022 and Qt

1. Install Visual Studio 2022 with **Desktop development with C++**, including the
   Windows SDK, CMake and Ninja.
2. Install a Qt 6 **MSVC 2022 64-bit** desktop kit using the Qt installer.
3. Install nlohmann/json using vcpkg: `vcpkg install nlohmann-json:x64-windows`.
4. Open **x64 Native Tools Command Prompt for VS 2022** in the repository.

Example paths below must be replaced by your Qt and vcpkg installations:

```bat
set PATH=C:/Qt/6.8.3/msvc2022_64/bin;%PATH%
cmake --preset desktop -DCMAKE_PREFIX_PATH=C:/Qt/6.8.3/msvc2022_64 -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build --preset desktop
ctest --preset desktop
build\desktop\bin\multra-desktop.exe
build\desktop\bin\multra-cli.exe 42
```

Put the Qt kit's `bin` on PATH before testing as well as launching, or deploy the DLLs
beside the executables. Use the same compiler/architecture for Qt and MULTRA.
MinGW Qt binaries are not compatible with an MSVC application.

### Local presets instead of repeated command-line flags

To avoid passing `CMAKE_PREFIX_PATH` and `CMAKE_TOOLCHAIN_FILE` on every configure,
copy [`CMakeUserPresets.windows.example.json`](CMakeUserPresets.windows.example.json) to
`CMakeUserPresets.json` in the repository root and replace the example Qt and vcpkg paths
with your own. That file is per-developer and is deliberately git-ignored; do not commit
machine paths. It adds `win-desktop`, `win-release` and `win-headless`, which inherit the
checked-in `desktop`, `release` and `headless` presets and build into separate directories:

```bat
cmake --preset win-desktop
cmake --build --preset win-desktop
ctest --preset win-desktop
build\win-desktop\bin\multra-desktop.exe
```

The test presets prepend the Qt kit's `bin` to `PATH` so CTest resolves the Qt DLLs
without a separately prepared shell. Visual Studio 2022 reads both preset files when the
repository folder is opened, and lists these presets in its configuration selector.

For a local runnable folder, build Release and use the matching Qt kit's `windeployqt`:

```bat
cmake --preset release -DCMAKE_PREFIX_PATH=C:/Qt/6.8.3/msvc2022_64 -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build --preset release
C:/Qt/6.8.3/msvc2022_64/bin/windeployqt.exe --release build/release/bin/multra-desktop.exe
```

**Thai line breaks (D245).** Qt loads libthai at run time, as `thai.dll` only, to find Thai line
breaks. Without it a Thai sentence breaks only at its spaces, and `properties-layout-ui` fails with
"Qt has no Thai line breaking here". The fix, as `native.yml`'s "Install Thai line breaking" step does it:
1. Install MSYS2's `mingw-w64-ucrt-x86_64-libthai`.
2. Copy `libthai-0.dll` as `thai.dll`, with `libdatrie-1.dll`, into a folder on `PATH`.
3. Set `LIBTHAI_DICTDIR` to the folder holding `thbrk.tri`.

The editor finds the dictionary by itself in `bin/data/libthai/`, where the package puts it.

Keep `bin/data/` beside the executable. This is a developer deployment folder;
the signed installer, file association and clean-machine acceptance remain M7.

## Linux — Ubuntu 24.04

```bash
sudo apt-get update
sudo apt-get install g++ cmake ninja-build nlohmann-json3-dev qt6-base-dev libproj-dev lld
cmake --preset desktop
cmake --build --preset desktop
ctest --preset desktop
./build/desktop/bin/multra-desktop --language th
```

`lld` is optional: when the compiler accepts `-fuse-ld=lld` it links every executable
(about 7x faster than GNU ld on this tree); `-DMULTRA_LLD=OFF` keeps the default linker.

In displayless CI the desktop test uses `QT_QPA_PLATFORM=offscreen`. Normal launches
use your desktop session's platform plugin to display a visible window.

### Windows host without a Linux box: WSL2

The same commands work in WSL2 Ubuntu, which needs no administrator rights and no reboot, and keeps
the build off the Windows drive. Verified 2026-09-21 on Ubuntu 26.04 with `g++ 15.2.0`,
`cmake 4.2.3`, `ninja 1.13.2`, Qt 6 Widgets and `nlohmann-json3-dev` — headless 21/21 and
desktop 30/30 green.

Two things to know:

- **Do not use ninja's default parallelism for the Qt build.** Ninja reads `nproc` (12 on the
  machine this was verified on) but WSL inherits only a share of the host RAM, and the desktop
  build is killed with exit code 15 rather than reporting an error. Build with `-- -j 3`, or raise
  the WSL memory limit in `.wslconfig`.
- WSL is displayless, so the desktop suite needs `QT_QPA_PLATFORM=offscreen` — the checked-in
  `desktop` test preset does not set it.

A WSL build is a Linux build. It is **not** evidence about the Windows MSVC path above, which
stays the platform the owner actually draws on.

## Headless, custom paths and installation

```bash
cmake --preset headless
cmake --build --preset headless
ctest --preset headless
./build/headless/bin/multra-cli --seed 42
cmake --install build/headless --prefix ./out/install
./out/install/bin/multra-cli 42
```

If nlohmann/json has no package config, set
`-DMULTRA_JSON_INCLUDE_DIR=/path/to/include` (containing `nlohmann/`).
No dependency is downloaded silently during configuration. For a custom Qt install
use `-DCMAKE_PREFIX_PATH=/path/to/Qt/kit`.

Presets use Ninja: executables are under `build/<preset>/bin/`. Visual Studio or Ninja
Multi-Config generators add `Debug/` or `Release/` after `bin/`; use `--config Release`
when building and `ctest -C Release` when testing. Runtime data follows that directory.
Every build mirrors `data/` there, including removed or renamed catalog files. The generated
destination must be inside the build tree and separate from the source. `data/proj/` is preserved
because `multra-proj-data` manages that subtree separately, including desktop builds of CLI alone.

## Checks and sanitizers

```bash
cmake --build build/headless --target check
cmake --preset sanitizer
cmake --build --preset sanitizer
ctest --preset sanitizer
```

The sanitizer option applies to GCC/Clang and runs in Linux CI. ASan, UBSan and explicit
`float-cast-overflow` stop at the first diagnostic; out-of-range signal group numbers must be
rejected before conversion to `int`. Tests use explicit failure checks, so
Release builds do not disable them through `NDEBUG`. Strict floating-point flags
disable fast-math and contraction; they do not promise bitwise identity across different
math libraries. Preserve toolchain/version metadata for future report runs.

Qt licensing depends on the modules and distribution arrangement; consult the
[Qt module documentation](https://doc.qt.io/qt-6/qtwidgets-index.html#licenses).
No commercial license or redistribution arrangement is assumed by this repository: MULTRA is
GPL-3.0-or-later and links Qt dynamically under the LGPL-3.0. A package must carry
[THIRD-PARTY-NOTICES.md](../THIRD-PARTY-NOTICES.md) and the texts in `licenses/`; the package
workflow copies them with each component's copyright file (D285).

## Network editor

`multra-desktop` opens the editor (add `--language th` for Thai).
`--scenario path/to/file.json` opens either file kind in it — an editor project or a bare M0
authoring scenario such as `data/scenarios/crossing.json`. `--editor` is still accepted and
does nothing; M1.24 retired the separate M0 harness window. See
[NETWORK_EDITOR.md](reference/NETWORK_EDITOR.md) for drawing, images and saving.
The headless build includes document/history tests but does not build Qt editor tests.

## OSM desktop dependencies

The desktop additionally needs Qt Network and PROJ (CMake target `PROJ::proj`).
Ubuntu: install `qt6-base-dev libproj-dev`; Windows/vcpkg: `proj:x64-windows` beside
`nlohmann-json:x64-windows`. Add vcpkg's `installed/x64-windows/bin` to PATH for tests
when applocal is disabled. Headless builds have no PROJ/Qt Network dependency.
CMake copies/installs `proj.db` into `bin/data/proj`; desktop startup selects that
packaged database: the database in `--data-dir/proj` when present, otherwise the executable's
`data/proj` even with a custom content directory. Windows packaging copies dependency DLLs and uses
windeployqt for Qt Network/TLS. System Linux packages need Qt Network and libproj
runtimes. Network tests use a local fake server and make no live OSM requests.

## Notes from the owner's machines

Moved from NEXT on 2026-10-15; dated, machine-specific.

- **Build on this machine:** MSVC 14.51, Ninja and Qt 6.8.3 on D: (vcvars64, then
  `cmake --preset desktop`) gives Windows evidence.
  - **WSL is not installed here** (`wsl.exe -l -v`, 2026-10-03). Linux evidence comes from CI
    (`native.yml`) or another Linux/WSL machine.
  - On a 3.7 GB WSL machine, `ar` on `/mnt/c` failed with "Cannot allocate memory" at full
    parallelism; use `-j 4` there. On another, a one-file edit waited 150 s (134 s linking the 101 MB Debug
    `multra-tests` on `/mnt/c`), and an unbounded `-j` hung a 3 GB WSL. The remedy: a build
    tree on ext4 via an untracked `CMakeUserPresets.json`, `CMAKE_JOB_POOLS=link=1` with
    `CMAKE_JOB_POOL_LINK=link`, `-j2`, and `--target` only what the change needs.
- **Timing on this machine:** the i5-12450H is hybrid. The same Debug run is 26–29 s CPU on
  P-cores (`ProcessorAffinity` 0xFF) and 89–104 s on E-cores (0xF00), 2026-10-04. Pin to one
  core type before comparing. Wall time under ctest swung 24–90 s on unchanged code.
