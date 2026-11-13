#pragma once

namespace multra {
// The newest project schema the reader accepts; tests name kLatestSchema + 1 as the first unknown one.
// Its own header so a schema step rebuilds only the files that read it, not every includer of
// document.hpp (541 objects; 10 of 16 document.hpp edits from 2026-10-28 changed only this value).
inline constexpr int kLatestSchema = 75;
}
