#include "link_model.hpp"
#include <algorithm>

namespace multra {
double MesoCurve::value(double t) const {
    if (at.empty()) return 0;
    if (t <= 0) return at.front();
    const double k = t / step;
    const auto i = static_cast<std::size_t>(k);
    if (i + 1 >= at.size()) return at.back();
    return at[i] + (k - static_cast<double>(i)) * (at[i + 1] - at[i]);
}
double MesoCurve::time(double n) const {
    if (at.empty() || n <= at.front()) return 0;
    const auto it = std::lower_bound(at.begin(), at.end(), n);
    if (it == at.end()) return static_cast<double>(at.size() - 1) * step;
    const auto i = static_cast<std::size_t>(it - at.begin());
    return (static_cast<double>(i - 1) + (n - at[i - 1]) / (at[i] - at[i - 1])) * step;
}
double mesoStepCapacity(const MesoLink& l, double dt) { return l.capacityPerLane * l.lanes * dt / 3600.0; }
double mesoStorage(const MesoLink& l) { return l.jamDensity * l.lanes * l.length / 1000.0; }
double mesoSending(const MesoLink& l, const MesoCurve& up, const MesoCurve& down, double t, double dt) {
    if (!(l.freeSpeed > 0)) return 0;
    return std::max(0.0, std::min(up.value(t + dt - l.length / l.freeSpeed) - down.value(t), mesoStepCapacity(l, dt)));
}
double mesoReceiving(const MesoLink& l, const MesoCurve& up, const MesoCurve& down, double t, double dt) {
    const double w = mesoWaveSpeed(l);
    const double downstream = w > 0 ? down.value(t + dt - l.length / w) : 0;
    return std::max(0.0, std::min(downstream + mesoStorage(l) - up.value(t), mesoStepCapacity(l, dt)));
}
}
