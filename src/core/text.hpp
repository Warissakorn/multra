#pragma once
#include <string>

namespace multra {
// The one rule for a blank id or text field: empty or only spaces, tabs and line breaks (D600).
inline bool isBlank(const std::string& s) { return s.find_first_not_of(" \t\r\n") == std::string::npos; }
}
