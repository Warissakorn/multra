#include "critical.hpp"
#include <algorithm>
#include <cmath>

namespace multra {
namespace {
constexpr double kTouch = 1e-9; // greens that only touch do not overlap
struct Best { double y{}, g{}; std::vector<std::size_t> chain; };
bool better(const Best& a, const Best& b) {
    if (std::abs(a.y - b.y) > 1e-12) return a.y > b.y;
    return a.g < b.g;
}
}
std::optional<CriticalResult> criticalRatio(const std::vector<CriticalInterval>& intervals, double cycle) {
    if (intervals.empty() || !(cycle > 0)) return std::nullopt;
    std::optional<Best> best;
    // Cut the circle at each interval's start in turn: that interval comes first, and the others that
    // fit between its end and its next start form a weighted interval scheduling problem on a line.
    for (std::size_t first = 0; first < intervals.size(); ++first) {
        const auto& f = intervals[first];
        struct Item { double from, to; std::size_t index; };
        std::vector<Item> items;
        for (std::size_t i = 0; i < intervals.size(); ++i) {
            if (i == first) continue;
            const double from = std::fmod(std::fmod(intervals[i].start - f.start, cycle) + cycle, cycle);
            const double to = from + intervals[i].green;
            if (from >= f.green - kTouch && to <= cycle + kTouch) items.push_back({from, to, i});
        }
        std::sort(items.begin(), items.end(), [](const Item& a, const Item& b) { return a.to < b.to; });
        // table[j]: the best chain among the first j items (by end).
        std::vector<Best> table(items.size() + 1);
        for (std::size_t j = 1; j <= items.size(); ++j) {
            const auto& it = items[j - 1];
            std::size_t p = j - 1;
            while (p > 0 && items[p - 1].to > it.from + kTouch) --p;
            Best take = table[p];
            take.y += intervals[it.index].flowRatio; take.g += intervals[it.index].effectiveGreen;
            take.chain.push_back(it.index);
            table[j] = better(take, table[j - 1]) ? take : table[j - 1];
        }
        Best chain = table.back();
        chain.y += f.flowRatio; chain.g += f.effectiveGreen;
        chain.chain.insert(chain.chain.begin(), first);
        if (!best || better(chain, *best)) best = chain;
    }
    if (!(best->g > 0)) return std::nullopt;
    return CriticalResult{best->chain, best->y, cycle - best->g, cycle / best->g * best->y};
}
}
