#pragma once
#include <functional>
#include <utility>
#include <vector>

namespace multra {
// D522: the selection hub the object tabs listen to, so the window publishes instead of reaching into
// each tab's table (the pattern Vissim's lists, Aimsun's tables and SUMO's inspector share: one
// selection, many views). Today it carries "the canvas selection was cleared"; a panel that wants it
// subscribes once, when it is built.
class ObjectSelection {
public:
    void onCleared(std::function<void()> listener) { cleared_.push_back(std::move(listener)); }
    void clear() const { for (const auto& listener : cleared_) listener(); }
private:
    std::vector<std::function<void()>> cleared_;
};
}
