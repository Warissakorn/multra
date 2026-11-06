#include "trip_generation.hpp"

namespace multra {
double tripEnd(const TripSide& side, const std::vector<double>& v) {
    if (side.method == TripSide::Method::crossClassification) {
        double t = 0;
        for (const auto& c : side.categories) t += v[c.attribute] * c.rate; // Eq. (4.16)
        return t;
    }
    double t = side.constant;
    for (const auto& term : side.terms) t += term.coefficient * v[term.attribute];
    return t;
}
TripGeneration generateTrips(const std::vector<std::vector<double>>& values, const std::vector<TripPurpose>& purposes) {
    TripGeneration out;
    for (std::size_t p = 0; p < purposes.size(); ++p) {
        TripPurposeResult r;
        r.id = purposes[p].id;
        for (std::size_t z = 0; z < values.size(); ++z) {
            const double prod = tripEnd(purposes[p].productions, values[z]);
            const double attr = tripEnd(purposes[p].attractions, values[z]);
            if (prod < 0) out.issues.push_back({TripIssue::Code::negative, p, z, false});
            if (attr < 0) out.issues.push_back({TripIssue::Code::negative, p, z, true});
            r.productions.push_back(prod);
            r.attractionsRaw.push_back(attr);
            r.totalProductions += prod;
            r.totalAttractionsRaw += attr;
        }
        // Eq. (4.14): the productions' total is taken as correct; attractions scale to it (§4).
        if (r.totalAttractionsRaw > 0) r.factor = r.totalProductions / r.totalAttractionsRaw;
        else if (r.totalProductions > 0) out.issues.push_back({TripIssue::Code::balanceImpossible, p, std::nullopt, false});
        for (const double a : r.attractionsRaw) {
            const double balanced = r.factor ? a * *r.factor : 0.0;
            r.attractions.push_back(balanced);
            r.totalAttractions += balanced;
        }
        out.purposes.push_back(std::move(r));
    }
    if (!out.issues.empty()) out.purposes.clear();
    return out;
}
}
