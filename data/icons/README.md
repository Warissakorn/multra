# Application identity

`trafficsim.svg` is the editable master: a T road with a turning arrow, in the editor
accent blue. `wordmark.svg` uses the current working name; it does not settle D11's
future naming decision. Both are original project assets under the repository's MIT license.

Committed PNG sizes (16–512 px) and the multi-size ICO are generated from the master.
Normal CMake builds need no image generator, SVG plugin or rasterization dependency.
Qt embeds the PNGs; Windows embeds the ICO; Linux installs the desktop entry and
hicolor icons. Text and parameter labels remain native vector text.
