#pragma once

// M13 A2 (docs/reference/NODE_ANALYSIS.md §11, D373): back of queue and queue storage ratio, HCM 7
// Ch. 31 §4 (PDF p. 3016-3039), for the basic arrival-departure polygon (Exhibit 31-25), which
// PDF p. 3028 applies to protected exclusive, split-phased and shared through+right lane groups:
// every lane group A2 analyses. No initial queue, so Q3 = 0. Pure; US units inside, as the book.
namespace multra {
// Eq. 31-132 (PDF p. 3023): S_a = 0.90 (25.6 + 0.47 S_pl), mi/h.
double approachSpeed(double speedLimitMph);
// Eq. 31-131 (PDF p. 3022): d_a = [1.47 (S_a - S_s)]^2 / (2 (1.47 S_a)) (1/r_a + 1/r_d), s, with
// S_s 5.0 mi/h, r_a 3.5 and r_d 4.0 ft/s^2.
double accelerationDelay(double approachSpeedMph);
struct QueueInput {
    double flowRate{};          // v, veh/h for the lane group
    int lanes{1};
    double saturation{};        // s, veh/h/ln
    double effectiveGreen{}, cycle{}, arrivalsOnGreen{}; // g, C (s) and P
    double capacity{}, incrementalDelay{}; // c (veh/h) and d2 (s/veh) from the Ch. 19 result
    double accelerationDelay{}; // d_a, s
    double percentile{1.64};    // z: 1.64 for the 95th percentile (PDF p. 3038)
    double filtering{1.0};      // I, isolated
};
struct BackOfQueue {
    double fullStopTime{};               // t_f, s (Eqs. 31-137, 31-138)
    double first{}, second{}, third{};   // Q1 (Eqs. 31-139 to 31-141), Q2 (Eq. 31-142), Q3 = 0
    double mean{}, percentileQueue{};    // Q (Eq. 31-149) and Q% (Eqs. 31-150, 31-151, 31-153), veh/ln
};
BackOfQueue backOfQueue(const QueueInput&);
// Eq. 31-155 (PDF p. 3038): L_h = 25 (1 - 0.01 P_HV) + 0.01 · 45 · P_HV, ft/veh.
double queueSpacing(double heavyPercent);
// Eqs. 31-154 and 31-156: R_Q = L_h Q / L_a, with L_a in ft.
double storageRatio(double queue, double heavyPercent, double storageFeet);
}
