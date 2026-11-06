#include "trip_distribution.hpp"
#include <algorithm>
#include <cmath>

namespace multra {
namespace {
double sum(const std::vector<double>& v) { double s = 0; for (const double x : v) s += x; return s; }
void correctRows(FurnessResult& r, const std::vector<double>& targets) {
    for (std::size_t i = 0; i < r.trips.size(); ++i) {
        const double s = sum(r.trips[i]);
        const double f = targets[i] > 0 && s > 0 ? targets[i] / s : 0.0;
        for (auto& x : r.trips[i]) x *= f;
        r.a[i] *= f;
    }
}
void correctColumns(FurnessResult& r, const std::vector<double>& targets) {
    for (std::size_t j = 0; j < targets.size(); ++j) {
        double s = 0;
        for (const auto& row : r.trips) s += row[j];
        const double f = targets[j] > 0 && s > 0 ? targets[j] / s : 0.0;
        for (auto& row : r.trips) row[j] *= f;
        r.b[j] *= f;
    }
}
}
double tripEndError(const DistributionMatrix& t, const std::vector<double>& rows, const std::vector<double>& columns) {
    double error = 0;
    for (std::size_t i = 0; i < rows.size(); ++i)
        if (rows[i] > 0) error = std::max(error, std::abs(sum(t[i]) - rows[i]) / rows[i]);
    for (std::size_t j = 0; j < columns.size(); ++j) {
        if (!(columns[j] > 0)) continue;
        double s = 0;
        for (const auto& row : t) s += row[j];
        error = std::max(error, std::abs(s - columns[j]) / columns[j]);
    }
    return error;
}
FurnessResult furness(DistributionMatrix prior, const std::vector<double>& rows, const std::vector<double>& columns,
                      const DistributionSettings& settings) {
    FurnessResult r{std::move(prior), std::vector<double>(rows.size(), 1.0), std::vector<double>(columns.size(), 1.0)};
    r.error = tripEndError(r.trips, rows, columns);
    const int cap = settings.fixedPasses ? *settings.fixedPasses : settings.maxPasses;
    while (r.passes < cap) {
        if (!settings.fixedPasses && r.error <= settings.target) break;
        correctRows(r, rows);
        correctColumns(r, columns);
        ++r.passes;
        r.error = tripEndError(r.trips, rows, columns);
    }
    r.converged = r.error <= settings.target;
    return r;
}
FurnessResult originConstrained(DistributionMatrix weights, const std::vector<double>& rows) {
    FurnessResult r{std::move(weights), std::vector<double>(rows.size(), 1.0), std::vector<double>(rows.size(), 1.0)};
    correctRows(r, rows);
    r.passes = 1;
    r.error = tripEndError(r.trips, rows, {});
    r.converged = true;
    return r;
}
namespace {
// D3 (§15.1): T_ij = P_i A_j f_ij / sum_k A_k f_ik in one row normalisation.
DistributionResult distributeFromOrigins(const DistributionInput& in) {
    const auto n = in.productions.size();
    DistributionResult out;
    std::vector<double> p = in.productions;
    out.intrazonal.assign(n, 0.0);
    for (std::size_t i = 0; i < n; ++i) { out.intrazonal[i] = in.intrazonalShare * p[i]; p[i] -= out.intrazonal[i]; }
    DistributionMatrix w(n, std::vector<double>(n, 0.0));
    for (std::size_t i = 0; i < n; ++i)
        for (std::size_t j = 0; j < n; ++j)
            if ((i != j || in.useDiagonal) && std::isfinite(in.costs[i][j])) w[i][j] = in.attractions[j] * std::exp(-in.beta * in.costs[i][j]);
    for (std::size_t i = 0; i < n; ++i)
        if (p[i] > 0 && !(sum(w[i]) > 0)) out.issues.push_back({DistributionIssue::Code::rowImpossible, i});
    if (!out.issues.empty()) return out;
    auto r = originConstrained(std::move(w), p);
    for (std::size_t i = 0; i < n; ++i) r.trips[i][i] += out.intrazonal[i];
    out.trips = std::move(r.trips); out.a = std::move(r.a); out.b = in.attractions;
    out.passes = r.passes; out.error = r.error; out.converged = r.converged;
    return out;
}
}
DistributionResult distributeTrips(const DistributionInput& in, const DistributionSettings& settings) {
    if (in.constraint == DistributionConstraint::origin) return distributeFromOrigins(in);
    const auto n = in.productions.size();
    DistributionResult out;
    if (std::abs(sum(in.productions) - sum(in.attractions)) > 1e-9 * std::max(sum(in.productions), sum(in.attractions))) {
        out.issues.push_back({DistributionIssue::Code::unbalanced, 0});
        return out;
    }
    // Choice 2a: intrazonal trips leave the gravity model as a share of the smaller trip end.
    std::vector<double> p = in.productions, q = in.attractions;
    out.intrazonal.assign(n, 0.0);
    for (std::size_t i = 0; i < n; ++i) {
        out.intrazonal[i] = in.intrazonalShare * std::min(p[i], q[i]);
        p[i] -= out.intrazonal[i]; q[i] -= out.intrazonal[i];
    }
    DistributionMatrix f(n, std::vector<double>(n, 0.0));
    double total = 0;
    for (std::size_t i = 0; i < n; ++i)
        for (std::size_t j = 0; j < n; ++j) {
            if ((i == j && !in.useDiagonal) || !std::isfinite(in.costs[i][j])) continue;
            f[i][j] = std::exp(-in.beta * in.costs[i][j]);
            total += f[i][j];
        }
    // §4 choice 5a: a positive end with nothing reachable at the other end is refused before any pass.
    for (std::size_t i = 0; i < n; ++i) {
        bool row = false, column = false;
        for (std::size_t k = 0; k < n; ++k) { row = row || (f[i][k] > 0 && q[k] > 0); column = column || (f[k][i] > 0 && p[k] > 0); }
        if (p[i] > 0 && !row) out.issues.push_back({DistributionIssue::Code::rowImpossible, i});
        if (q[i] > 0 && !column) out.issues.push_back({DistributionIssue::Code::columnImpossible, i});
    }
    if (!out.issues.empty()) return out;
    // Example 5.4's seed: f scaled to the total, so b carries T / sum f.
    const double trips = sum(p), scale = total > 0 ? trips / total : 0.0;
    for (auto& row : f) for (auto& x : row) x *= scale;
    auto r = furness(std::move(f), p, q, settings);
    for (auto& x : r.b) x *= scale;
    for (std::size_t i = 0; i < n; ++i) r.trips[i][i] += out.intrazonal[i];
    out.trips = std::move(r.trips); out.a = std::move(r.a); out.b = std::move(r.b);
    out.passes = r.passes; out.error = r.error; out.converged = r.converged;
    return out;
}
double meanCost(const DistributionMatrix& trips, const DistributionMatrix& costs) {
    double t = 0, tc = 0;
    for (std::size_t i = 0; i < trips.size(); ++i)
        for (std::size_t j = 0; j < trips[i].size(); ++j)
            if (std::isfinite(costs[i][j])) { t += trips[i][j]; tc += trips[i][j] * costs[i][j]; }
    return t > 0 ? tc / t : 0.0;
}
std::vector<double> tripLengthBands(const DistributionMatrix& trips, const DistributionMatrix& costs, double width) {
    std::vector<double> bands;
    for (std::size_t i = 0; i < trips.size(); ++i)
        for (std::size_t j = 0; j < trips[i].size(); ++j) {
            if (!std::isfinite(costs[i][j]) || !(trips[i][j] > 0)) continue;
            const double k = costs[i][j] <= width ? 0.0 : std::ceil(costs[i][j] / width) - 1;
            const auto band = static_cast<std::size_t>(k);
            if (bands.size() <= band) bands.resize(band + 1, 0.0);
            bands[band] += trips[i][j];
        }
    return bands;
}
}
