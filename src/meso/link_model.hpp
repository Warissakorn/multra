#pragma once
#include "graph.hpp"

// M11.3c link model (docs/reference/MESO_NETWORK.md §4, D298): Yperman (2007), "The Link Transmission
// Model for Dynamic Network Loading", KU Leuven doctoral thesis, §3.1 and §4.6. A link's state is its
// cumulative counts at the two ends (Newell 1993); the sending and receiving volumes follow from them
// with a triangular diagram. Counts are PCU; capacityPerLane is PCU/h per lane, freeSpeed m/s,
// jamDensity PCU/km per lane, length m.
namespace multra {
// Cumulative counts at k * step, k = 0, 1, ...: linear between steps (Eq. 3.2), at[0] before 0 and the
// last value after the last step.
struct MesoCurve {
    double step{};
    std::vector<double> at;
    double value(double t) const;
    // The earliest time the count reaches n (the inverse of Eq. 3.2); the last step's time above it.
    double time(double n) const;
};
// The link's capacity in PCU per step dt, s: q_max * lanes * dt.
double mesoStepCapacity(const MesoLink&, double dt);
// Its storage, PCU: k_jam * lanes * L.
double mesoStorage(const MesoLink&);
// Sending volume over [t, t + dt] (Eq. 4.31): min(N_up(t + dt - L/v) - N_down(t), q_max dt).
double mesoSending(const MesoLink&, const MesoCurve& up, const MesoCurve& down, double t, double dt);
// Receiving volume over [t, t + dt] (Eq. 4.35, w taken positive): min(N_down(t + dt - L/w) +
// k_jam L - N_up(t), q_max dt). Without a wave speed the downstream term is 0, so only the storage
// counts; a link without capacity receives nothing.
double mesoReceiving(const MesoLink&, const MesoCurve& up, const MesoCurve& down, double t, double dt);
}
