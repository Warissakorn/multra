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
std::vector<double> nestedLogitShares(const std::vector<double>& v, const std::vector<int>& nestOf, const std::vector<double>& phi) {
    const auto none = -std::numeric_limits<double>::infinity();
    // Branches: one per nest, then one per alternative in no nest; W = -inf when no member is available.
    std::vector<double> top(phi.size(), none), w;
    for (std::size_t k = 0; k < v.size(); ++k)
        if (nestOf[k] >= 0 && std::isfinite(v[k])) top[nestOf[k]] = std::max(top[nestOf[k]], v[k]);
    std::vector<double> inner(v.size(), 0.0), sum(phi.size(), 0.0);
    for (std::size_t k = 0; k < v.size(); ++k)
        if (const int n = nestOf[k]; n >= 0 && std::isfinite(v[k])) { inner[k] = std::exp((v[k] - top[n]) / phi[n]); sum[n] += inner[k]; }
    for (std::size_t n = 0; n < phi.size(); ++n) w.push_back(std::isfinite(top[n]) ? top[n] + phi[n] * std::log(sum[n]) : none);
    std::vector<std::size_t> own;
    for (std::size_t k = 0; k < v.size(); ++k)
        if (nestOf[k] < 0) { own.push_back(k); w.push_back(v[k]); }
    const auto branch = logitShares(w);
    std::vector<double> shares(v.size(), 0.0);
    for (std::size_t k = 0; k < v.size(); ++k)
        if (const int n = nestOf[k]; n >= 0 && sum[n] > 0) shares[k] = branch[n] * inner[k] / sum[n];
    for (std::size_t b = 0; b < own.size(); ++b) shares[own[b]] = branch[phi.size() + b];
    return shares;
}
double logsum(const std::vector<double>& v, const std::vector<int>& nestOf, const std::vector<double>& phi) {
    const auto none = -std::numeric_limits<double>::infinity();
    std::vector<double> w;
    for (std::size_t n = 0; n < phi.size(); ++n) {
        double top = none;
        for (std::size_t k = 0; k < v.size(); ++k) if (nestOf[k] == static_cast<int>(n) && std::isfinite(v[k])) top = std::max(top, v[k]);
        if (!std::isfinite(top)) continue;
        double sum = 0;
        for (std::size_t k = 0; k < v.size(); ++k) if (nestOf[k] == static_cast<int>(n) && std::isfinite(v[k])) sum += std::exp((v[k] - top) / phi[n]);
        w.push_back(top + phi[n] * std::log(sum));
    }
    for (std::size_t k = 0; k < v.size(); ++k) if ((nestOf.empty() || nestOf[k] < 0) && std::isfinite(v[k])) w.push_back(v[k]);
    if (w.empty()) return none;
    const double top = *std::max_element(w.begin(), w.end());
    double sum = 0;
    for (const double x : w) sum += std::exp(x - top);
    return top + std::log(sum);
}
DistributionMatrix compositeCost(const std::vector<ModeAlternativeInput>& alternatives, const ModeNests& nests, double lambda) {
    const auto n = alternatives.empty() ? 0 : alternatives.front().costs.size();
    DistributionMatrix k(n, std::vector<double>(n, std::numeric_limits<double>::infinity()));
    std::vector<double> v(alternatives.size());
    for (std::size_t i = 0; i < n; ++i)
        for (std::size_t j = 0; j < n; ++j) {
            for (std::size_t a = 0; a < alternatives.size(); ++a) {
                const double c = alternatives[a].costs[i][j];
                v[a] = std::isfinite(c) ? alternatives[a].constant - lambda * c : std::numeric_limits<double>::quiet_NaN();
            }
            const double ls = logsum(v, nests.nestOf, nests.phi);
            if (std::isfinite(ls)) k[i][j] = -ls / lambda;
        }
    return k;
}
ModeSplit splitModes(const DistributionMatrix& trips, const std::vector<ModeAlternativeInput>& alternatives, const ModeNests& nests) {
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
            const auto shares = nests.nestOf.empty() ? logitShares(v) : nestedLogitShares(v, nests.nestOf, nests.phi);
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
