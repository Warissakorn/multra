#include "fctl_reference.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>
#include <utility>

namespace multra {
namespace {
// A minimal complex number: <complex> is not in the reviewed header set (check_architecture).
struct Complex {
    double re{}, im{};
    Complex(double r = 0, double i = 0) : re(r), im(i) {}
    double real() const { return re; }
    double imag() const { return im; }
    Complex& operator+=(Complex o) { re += o.re; im += o.im; return *this; }
    Complex& operator-=(Complex o) { re -= o.re; im -= o.im; return *this; }
    Complex& operator*=(Complex o) { return *this = Complex{re * o.re - im * o.im, re * o.im + im * o.re}; }
    friend Complex operator+(Complex a, Complex b) { return a += b; }
    friend Complex operator-(Complex a, Complex b) { return a -= b; }
    friend Complex operator*(Complex a, Complex b) { return a *= b; }
    friend Complex operator/(Complex a, Complex b) {
        const double d = b.re * b.re + b.im * b.im;
        return {(a.re * b.re + a.im * b.im) / d, (a.im * b.re - a.re * b.im) / d};
    }
};
double magnitude(Complex z) { return std::hypot(z.re, z.im); }
Complex expOf(Complex z) { const double m = std::exp(z.re); return {m * std::cos(z.im), m * std::sin(z.im)}; }
constexpr int kMaxIterations = 100000;

// Solves a * x = b in place by Gaussian elimination with partial pivoting.
std::vector<Complex> solve(std::vector<std::vector<Complex>> a, std::vector<Complex> b) {
    const auto n = b.size();
    for (std::size_t col = 0; col < n; ++col) {
        std::size_t pivot = col;
        for (std::size_t row = col + 1; row < n; ++row)
            if (magnitude(a[row][col]) > magnitude(a[pivot][col])) pivot = row;
        if (magnitude(a[pivot][col]) == 0.0) throw std::runtime_error("FCTL system is singular");
        std::swap(a[col], a[pivot]);
        std::swap(b[col], b[pivot]);
        for (std::size_t row = col + 1; row < n; ++row) {
            const Complex f = a[row][col] / a[col][col];
            for (std::size_t k = col; k < n; ++k) a[row][k] -= f * a[col][k];
            b[row] -= f * b[col];
        }
    }
    std::vector<Complex> x(n);
    for (std::size_t i = n; i-- > 0;) {
        Complex sum = b[i];
        for (std::size_t k = i + 1; k < n; ++k) sum -= a[i][k] * x[k];
        x[i] = sum / a[i][i];
    }
    return x;
}
} // namespace

FctlResult fctlPoisson(const FctlInput& in) {
    const int g = in.green, r = in.red, c = g + r;
    const double mu = in.mu;
    if (g < 1 || r < 0 || !(mu > 0) || !(c * mu < g))
        throw std::invalid_argument("FCTL needs green >= 1, red >= 0 and 0 < (green + red) * mu < green");
    FctlResult out;
    const auto Y = [mu](Complex z) { return expOf(mu * (z - 1.0)); };
    // Roots of z^g = Y(z)^c on and within the unit circle (Eq. 5.2), z_0 = 1.
    std::vector<Complex> roots(static_cast<std::size_t>(g));
    for (int k = 0; k < g; ++k) {
        const double angle = 2.0 * std::numbers::pi * k / g;
        const Complex w{std::cos(angle), std::sin(angle)};
        Complex z = 0.0;
        int it = 0;
        for (; it < kMaxIterations; ++it) {
            const Complex next = w * expOf(c * mu / g * (z - 1.0));
            const bool done = magnitude(next - z) < 1e-13;
            z = next;
            if (done) break;
        }
        if (it == kMaxIterations) throw std::runtime_error("FCTL root did not converge");
        out.iterations = std::max(out.iterations, it + 1);
        roots[static_cast<std::size_t>(k)] = z;
    }
    // The g unknowns q_0 .. q_{g-1} (Eq. 3.9, 3.11).
    const auto n = static_cast<std::size_t>(g);
    std::vector<std::vector<Complex>> a(n, std::vector<Complex>(n, 1.0));
    std::vector<Complex> b(n, 0.0);
    b[0] = (g - c * mu) / (1.0 - mu);
    for (std::size_t i = 1; i < n; ++i) {
        const Complex zeta = roots[i] / Y(roots[i]);
        Complex power = 1.0;
        for (std::size_t j = 0; j < n; ++j, power *= zeta) a[i][j] = power;
    }
    const auto q = solve(std::move(a), std::move(b));
    double weighted = 0;
    for (std::size_t j = 0; j < n; ++j) {
        if (std::abs(q[j].imag()) > 1e-9 || q[j].real() < -1e-9 || q[j].real() > 1 + 1e-9)
            throw std::runtime_error("FCTL q_j is not a probability");
        out.q.push_back(q[j].real());
        weighted += static_cast<double>(j) * q[j].real();
    }
    // Eq. 3.13 and 3.17 with the Poisson variance sigma^2 = mu.
    const double var = mu, slack = g - c * mu;
    out.meanOverflow = (c * var + r * r * mu * mu - g * g * (1 - mu) * (1 - mu)) / (2 * slack)
        - var / (2 * (1 - mu)) + (1 - mu) / 2 + (1 - mu) * (1 - mu) / slack * weighted;
    out.meanDelay = r / (2 * c * mu * (1 - mu)) * (var / (1 - mu) + r * mu + 2 * out.meanOverflow);
    return out;
}
} // namespace multra
