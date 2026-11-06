#include "mode_choice.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace multra {
std::vector<double> logitShares(const std::vector<double>& v) {
    std::vector<double> shares(v.size(), 0.0);
    double top = -std::numeric_limits<double>::infinity();
    for (const double x : v) if (std::isfinite(x)) top = std::max(top, x);
    if (!std::isfinite(top)) return shares;
    double total = 0;
    for (std::size_t k = 0; k < v.size(); ++k)
        if (std::isfinite(v[k])) { shares[k] = std::exp(v[k] - top); total += shares[k]; }
    for (auto& s : shares) s /= total;
    return shares;
}
ModeSplit splitModes(const DistributionMatrix& trips, const std::vector<ModeAlternativeInput>& alternatives) {
    const auto n = trips.size();
    ModeSplit out;
    std::vector<DistributionMatrix> persons(alternatives.size(), DistributionMatrix(n, std::vector<double>(n, 0.0)));
    std::vector<double> v(alternatives.size());
    for (std::size_t i = 0; i < n; ++i)
        for (std::size_t j = 0; j < n; ++j) {
            if (!(trips[i][j] > 0)) continue;
            for (std::size_t k = 0; k < alternatives.size(); ++k) {
                const auto& a = alternatives[k];
                const double c = a.costs[i][j];
                v[k] = std::isfinite(c) ? a.constant + a.timeCoefficient * c : std::numeric_limits<double>::quiet_NaN();
            }
            const auto shares = logitShares(v);
            if (std::none_of(shares.begin(), shares.end(), [](double s) { return s > 0; })) { out.issues.push_back({i, j}); continue; }
            for (std::size_t k = 0; k < alternatives.size(); ++k) persons[k][i][j] = shares[k] * trips[i][j];
        }
    if (out.issues.empty()) out.persons = std::move(persons);
    return out;
}
DistributionMatrix directionalOd(const DistributionMatrix& t, double fPA, double fAP) {
    const auto n = t.size();
    DistributionMatrix od(n, std::vector<double>(n, 0.0));
    for (std::size_t i = 0; i < n; ++i)
        for (std::size_t j = 0; j < n; ++j) od[i][j] = fPA * t[i][j] + fAP * t[j][i];
    return od;
}
}
