#include "queue.hpp"
#include <algorithm>
#include <cmath>

namespace multra {
double approachSpeed(double speedLimitMph) { return 0.90 * (25.6 + 0.47 * speedLimitMph); }
double accelerationDelay(double sa) {
    constexpr double stopped = 5.0, accelerate = 3.5, decelerate = 4.0;
    if (!(sa > stopped)) return 0;
    const double drop = 1.47 * (sa - stopped);
    return drop * drop / (2 * 1.47 * sa) * (1 / accelerate + 1 / decelerate);
}
BackOfQueue backOfQueue(const QueueInput& in) {
    BackOfQueue out;
    const double n = in.lanes, g = in.effectiveGreen, cycle = in.cycle, red = cycle - g, p = in.arrivalsOnGreen, da = in.accelerationDelay;
    const double x = in.capacity > 0 ? in.flowRate / in.capacity : 0, xm = std::min(1.0, x);
    // Per lane, in veh/s; a flow over the lane capacity is set to it (PDF p. 3024).
    const double q = std::min(in.flowRate, in.capacity) / (3600 * n), s = in.saturation / 3600;
    if (red > 0 && g > 0 && q > 0) {
        const double qr = (1 - p) * q * cycle / red, qg = p * q * cycle / g;
        if (da <= (1 - p) * g * x) {
            out.fullStopTime = q * cycle * (1 - p - p * da / g) / (s * (1 - xm * p));  // Eq. 31-137
            out.first = qr * red + qg * (out.fullStopTime - da);                       // Eq. 31-139
        } else {
            out.fullStopTime = q * cycle * (1 - p) * (red - da) / (s * (red - xm * (1 - p) * g)); // Eq. 31-138
            out.first = qr * (red - da + out.fullStopTime);                                       // Eq. 31-140
        }
        out.first = std::max(0.0, out.first); // Eq. 31-141: Q1 = N_f
    }
    out.second = in.capacity / (3600 * n) * in.incrementalDelay; // Eq. 31-142, c_A = c
    out.third = 0;
    out.mean = out.first + out.second + out.third; // Eq. 31-149
    const double base = out.first + out.second;
    if (base > 0) {
        double factor = 1.0 + in.percentile * std::sqrt(in.filtering / base); // Eq. 31-153
        if (x >= 1) factor += 0.60 * std::pow(in.percentile, 0.24) * std::pow(g / cycle, 0.33) * (1.0 - std::exp(2 - 2 * x)); // Eq. 31-151
        out.percentileQueue = base * std::min(1.8, factor) + out.third; // Eq. 31-150
    }
    return out;
}
double queueSpacing(double heavyPercent) { return 25 * (1 - 0.01 * heavyPercent) + 0.01 * 45 * heavyPercent; }
double storageRatio(double queue, double heavyPercent, double storageFeet) {
    return storageFeet > 0 ? queueSpacing(heavyPercent) * queue / storageFeet : 0;
}
}
