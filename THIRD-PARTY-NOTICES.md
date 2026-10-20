# Third-party notices

MULTRA is licensed under the GNU General Public License, version 3 or later ([LICENSE](LICENSE)).
It uses the components below, each under its own licence. The package workflow
(`.github/workflows/package.yml`) copies this file, the licence texts in [`licenses/`](licenses/)
and each component's own copyright file into `licenses/` of every build it produces.

| Component | Licence | How it is used | Shipped in |
|---|---|---|---|
| [Qt 6](https://www.qt.io/) (Core, Gui, Widgets, Network) | LGPL-3.0 ([text](licenses/LGPL-3.0.txt)) | Linked dynamically | Windows package (DLLs placed by `windeployqt`); Linux uses the system's Qt |
| [PROJ](https://proj.org/) and its database `proj.db` | MIT; `proj.db` holds the [EPSG Dataset](https://epsg.org/terms-of-use.html), under the IOGP terms of use | Linked; `proj.db` copied to `bin/data/proj/` | Both packages (Windows also ships `proj.dll`) |
| Libraries vcpkg builds for PROJ (for example SQLite, libtiff, curl, zlib) | Each port's own licence | Linked dynamically | Windows package; their `copyright` files go to `licenses/vcpkg/` |
| [nlohmann/json](https://github.com/nlohmann/json) | MIT | Header-only, compiled into the executables | Both packages |
| [libthai](https://linux.thai.net/projects/libthai) and [libdatrie](https://linux.thai.net/projects/datrie) | LGPL-2.1-or-later ([text](licenses/LGPL-2.1.txt)) | Loaded at run time as `thai.dll` for Thai line breaks (D245), with the dictionary `thbrk.tri` | Windows package |
| [Noto Sans Thai](https://github.com/notofonts/thai) | SIL Open Font License 1.1 ([data/fonts/OFL.txt](data/fonts/OFL.txt)) | Bundled UI font, two static weights cut from the variable font | Both packages (`bin/data/fonts/`) |
| [OpenStreetMap](https://www.openstreetmap.org/copyright) tiles | Map data ODbL; tiles under the OSMF tile usage policy | Fetched at run time when a map background is on; the credit is shown on the map | Not shipped |

## Source for the LGPL components

The LGPL lets you replace these libraries with your own build. The Windows package links them
dynamically, so replacing the DLL is enough. Their sources:

- Qt 6.5.3, the version the Windows package is built with:
  <https://download.qt.io/archive/qt/6.5/6.5.3/single/>. Qt itself contains third-party code
  under other licences, listed at <https://doc.qt.io/qt-6/licenses-used-in-qt.html>.
- libthai and libdatrie, as packaged by MSYS2 (`mingw-w64-ucrt-x86_64-libthai`):
  <https://github.com/msys2/MINGW-packages> and upstream at <https://github.com/tlwg>.

## Earlier versions

Commits up to `2053141` (2026-10-20) were published under the MIT License. Those copies stay
available under MIT; later versions are GPL-3.0-or-later.
