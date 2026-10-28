#pragma once

// M13 A3 (docs/reference/NODE_ANALYSIS_A3.md, D376): HCM 7 Ch. 31 §3's procedure for a far turn served
// in the permitted or protected-permitted mode (Steps 1-6, PDF p. 2986-3002), with the permitted terms
// of Ch. 31 §2's lane-flow procedure (PDF p. 2953-2958) and Step 7 B's capacities (PDF p. 3010-3013).
// No pedestrians or bicycles, so fLpb = 1 (D375 4a). HCM's left turn is the far turn. Pure.
namespace multra {
// t_cg 4.5 s and t_fh 2.5 s (Eq. 31-100), sneakers n_s 2.0 per cycle (Eq. 31-119), E_L 1.05.
inline constexpr double kCriticalHeadway = 4.5, kFollowUpHeadway = 2.5, kSneakersPerCycle = 2.0, kProtectedFarEquivalent = 1.05;
// Eq. 31-100 (PDF p. 2992): s_p = v_o e^(-v_o t_cg/3600) / (1 - e^(-v_o t_fh/3600)), veh/h/ln; an
// opposing flow of 0 is taken as 0.1 veh/h.
double permittedSaturation(double opposingFlow);
// Eq. 31-103 (PDF p. 2994): n_q = 0.278 (g_p - g_u - g_f), at least 0.
double opposingArrivals(double permittedGreen, double unblockedGreen, double firstBlock);
// Eq. 31-102 (PDF p. 2994): E_L2 = (1 - (1 - P_lto)^n_q)/P_lto, at least E_L; as P_lto -> 0 it is n_q.
double queuedEquivalent(double opposingFarShare, double opposingArrivals);
// Eqs. 31-47 to 31-49 (PDF p. 2953) with fLpb = 1: (E - 1) P_lc + 1.
double modifiedEquivalent(double equivalent, double laneChange);
// Exhibit 31-12 (PDF p. 2987) as one rule on displayed times in one frame of the cycle: the window
// [windowStart, windowEnd] is when the subject far turn is green and a movement it yields to is green
// too; the opposing through's green starts at opposingStart <= windowStart; G_q2 is the opposing
// queue's displayed service time. G_U = windowEnd - max(windowStart, opposingStart + G_q2) and
// G_p = windowEnd - windowStart, each at least 0.
struct PermittedGreen {
    double displayed{}, unblockedDisplayed{}; // G_p, G_U
    double startupLost{}, extension{};        // l_1,p and e_p
    double effective{}, unblocked{};          // g_p (Eq. 31-94) and g_u (Eq. 31-95)
};
// l_1,p: l_1 when the window opens the subject's display, else max(0, min(l_1, gap + l_1 - e)) after a
// protected green ending `gap` s before it (Exhibit 31-12 note b; 0 when continuous). e_p: e when
// the display ends with the window, 0 when it continues into the protected green.
PermittedGreen permittedGreen(double windowStart, double windowEnd, double opposingStart, double opposingQueue,
                              bool opensDisplay, double gapAfterProtected, bool endsDisplay);
// Eqs. 31-96 to 31-99 (PDF p. 2990-2991): the time before the first far-turning vehicle blocks a
// shared lane, g_f, at most g_f,max. `farShare` P_L, `farFlow` v_lt veh/h; `oneLane`: the approach has
// one lane (Eq. 31-97), else Eq. 31-98.
double firstBlockTime(double displayedPermitted, double effectivePermitted, double startupLost, double farShare,
                      double farFlow, double cycle, bool oneLane);
// Eq. 31-60 (PDF p. 2958): expected sneakers per cycle in a shared lane, n_s*.
double sharedSneakers(double farShare);
// The three service periods of a permitted shared lane (PDF p. 2997-2999): g_f at s_th, g_diff
// (Eq. 31-107) and min(g_p - g_f, g_u).
struct SharedPeriods { double first{}, second{}, third{}; };
SharedPeriods sharedPeriods(double permittedGreen, double unblockedGreen, double firstBlock);
// s_sl/s_th for a permitted shared lane over g_p: Eq. 31-59's bracket (with E_L1,m, E_L2,m and the
// sneaker term 3600 n_s*/s_th) when `sneakersOverSaturation` is n_s*/s_th, or Eq. 31-122's (E_L1, E_L2,
// no sneaker term) when it is 0. `secondServed` is false with two or more opposing lanes (s_sl2 = 0).
double sharedPermittedShare(const SharedPeriods&, double permittedGreen, double farShare, double queuedEq,
                            double filteringEq, bool secondServed, double sneakersOverSaturation);
// Step 7 B (PDF p. 3011-3013), veh/h, with f_ms = f_sp = 1. An exclusive far-turn lane group:
// Eq. 31-119 (g_u s_l + 3600 n_s)/C N, plus g_l s_lt/C N when protected-permitted (Eq. 31-124).
double exclusivePermittedCapacity(double unblockedGreen, double permittedSaturation, double cycle, int lanes,
                                  double protectedGreen = 0, double protectedSaturation = 0);
// A shared far-turn lane: Eq. 31-121 (g_p s_sl + 3600 (1 + P_L))/C, plus g_l s_sl4/C when
// protected-permitted (Eq. 31-126).
double sharedPermittedCapacity(double permittedGreen, double sharedSaturation, double farShare, double cycle,
                               double protectedGreen = 0, double protectedSaturation = 0);
}
