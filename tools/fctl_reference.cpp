// multra-fctl-reference (D529, docs/reference/FCTL_REFERENCE.md): M6 S2-c's analytic reference,
// the fixed-cycle traffic-light queue with Poisson arrivals by van Leeuwaarden (2006)'s procedure.
#include "../src/eval/fctl_reference.hpp"
#include <cstdio>
#include <exception>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
constexpr const char* kUsage =
    "usage: multra-fctl-reference --saturation S --green-slots G --red-slots R --x X1,X2,...\n"
    "  S in veh/h sets the slot h = 3600 / S s; G and R are whole slots; each x gives\n"
    "  mu = x * G / (G + R) Poisson arrivals per slot. Prints the mean delay h * E D (s/veh).\n";

double number(const std::string& text, const char* option) {
    std::size_t used = 0;
    const double value = std::stod(text, &used);
    if (used != text.size()) throw std::invalid_argument(std::string("invalid value for ") + option);
    return value;
}
int whole(const std::string& text, const char* option) {
    std::size_t used = 0;
    const int value = std::stoi(text, &used);
    if (used != text.size()) throw std::invalid_argument(std::string("invalid value for ") + option);
    return value;
}
} // namespace

int main(int argc, char** argv) {
    try {
        double saturation = 0;
        int green = -1, red = -1;
        std::vector<double> xs;
        for (int i = 1; i < argc; ++i) {
            const std::string arg = argv[i];
            if (arg == "--help") { std::fputs(kUsage, stdout); return 0; }
            if (i + 1 >= argc) throw std::invalid_argument(arg + " needs a value");
            const std::string value = argv[++i];
            if (arg == "--saturation") saturation = number(value, "--saturation");
            else if (arg == "--green-slots") green = whole(value, "--green-slots");
            else if (arg == "--red-slots") red = whole(value, "--red-slots");
            else if (arg == "--x") {
                std::stringstream list(value);
                for (std::string item; std::getline(list, item, ',');) xs.push_back(number(item, "--x"));
            } else throw std::invalid_argument("unknown option " + arg);
        }
        if (!(saturation > 0)) throw std::invalid_argument("--saturation is required and must be positive");
        if (green < 0 || red < 0) throw std::invalid_argument("--green-slots and --red-slots are required");
        if (xs.empty()) throw std::invalid_argument("--x is required");
        const double h = 3600.0 / saturation;
        std::printf("{\"model\": \"fctl-poisson\", \"source\": \"vanLeeuwaarden06\", \"slotSeconds\": %.6f, "
                    "\"greenSlots\": %d, \"redSlots\": %d, \"rows\": [", h, green, red);
        for (std::size_t i = 0; i < xs.size(); ++i) {
            const double mu = xs[i] * green / (green + red);
            const auto result = multra::fctlPoisson({green, red, mu});
            std::printf("%s\n  {\"x\": %.4f, \"mu\": %.6f, \"flow\": %.3f, \"meanOverflow\": %.6f, "
                        "\"meanDelaySlots\": %.6f, \"referenceSeconds\": %.4f}",
                        i ? "," : "", xs[i], mu, mu * 3600.0 / h, result.meanOverflow, result.meanDelay,
                        h * result.meanDelay);
        }
        std::printf("\n]}\n");
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "multra-fctl-reference: %s\n%s", error.what(), kUsage);
        return 2;
    }
}
