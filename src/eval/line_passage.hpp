#pragma once

namespace multra {
// D653: a front exactly at a line has not passed it. STOP_LINES §2 puts the line where the front stops, and
// LINE_STOP (D652) rests it there. A line at a route's start is passed on insertion: a vehicle is placed there.
inline bool pastLine(double front, double line) { return front > line || (line == 0 && front >= 0); }
}
