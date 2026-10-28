#include "qap.hpp"
#include <algorithm>
#include <cmath>

namespace multra {
namespace {
struct Pass { double end{}, area{}; std::vector<double> queue; std::optional<double> zeroAfter; bool cleared{}; };
// One cycle from `start`, recording each period's end queue, the area, and the first time after the
// start of period `from` at which the queue is 0.
Pass run(const std::vector<QapPeriod>& periods, double start, std::size_t from) {
    Pass p;
    double q = start, sinceFrom = -1;
    for (std::size_t i = 0; i < periods.size(); ++i) {
        const auto& x = periods[i];
        if (i == from) sinceFrom = 0;
        const double w = x.service - x.arrival; // queue change rate while served (w_q, Eq. 31-116)
        double next = q, used = x.duration;
        if (x.service > 0 && w > 0 && q > 0 && q / w < x.duration) { // empties within the period
            used = q / w;
            next = 0;
            if (sinceFrom >= 0 && !p.zeroAfter) p.zeroAfter = sinceFrom + used;
        } else if (x.service > 0 && w >= 0 && q <= 0) {
            next = 0; used = 0;
            if (sinceFrom >= 0 && !p.zeroAfter) p.zeroAfter = sinceFrom;
        } else {
            next = std::max(0.0, q - w * x.duration);
        }
        p.area += 0.5 * (q + next) * used; // Eq. 31-115's trapezoid over t_t,i (Eq. 31-116)
        q = next;
        if (q <= 0) p.cleared = true;
        if (x.sneakers > 0 && q > 0) {
            q = std::max(0.0, q - x.sneakers);
            if (q <= 0 && sinceFrom >= 0 && !p.zeroAfter) p.zeroAfter = sinceFrom + x.duration; // g_ps ends with the period
        }
        if (q <= 0) p.cleared = true;
        p.queue.push_back(q);
        if (sinceFrom >= 0) sinceFrom += x.duration;
    }
    p.end = q;
    return p;
}
}
QueueAccumulation queueAccumulation(const std::vector<QapPeriod>& periods, std::size_t serviceFrom) {
    QueueAccumulation out;
    if (periods.empty()) return out;
    // Rotated to begin at `serviceFrom`, so the service time is read within one pass.
    const auto from = serviceFrom % periods.size();
    std::vector<QapPeriod> cycle(periods.begin() + static_cast<std::ptrdiff_t>(from), periods.end());
    cycle.insert(cycle.end(), periods.begin(), periods.begin() + static_cast<std::ptrdiff_t>(from));
    double arrivals = 0;
    for (const auto& x : cycle) arrivals += x.arrival * x.duration;
    double start = 0;
    Pass pass = run(cycle, start, 0);
    for (int i = 0; i < 100 && std::abs(pass.end - start) > 1e-9; ++i) {
        start = pass.end;
        pass = run(cycle, start, 0);
    }
    out.queue.assign(periods.size(), 0);
    for (std::size_t i = 0; i < cycle.size(); ++i) out.queue[(i + from) % periods.size()] = pass.queue[i];
    out.cleared = pass.cleared;
    out.serviceTime = pass.zeroAfter;
    out.uniformDelay = arrivals > 0 ? pass.area / arrivals : 0;
    return out;
}
}
