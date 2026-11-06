#pragma once
#include "trip_generation_panel.hpp"

namespace multra {
// M10.3 D2 (TRIP_DISTRIBUTION §8, D474): self-owned Results page for distribution, built as the Trip
// generation page: choices of rate set, deterrence set, cost matrix and skim mode; Run on a worker with
// its four stages; Deterrence sets… and Cost matrices…; one row per purpose and zone pair plus a
// totals row. Never a History step. It takes the Trip generation page's context.
class TripDistributionPanel final : public QWidget {
public:
    TripDistributionPanel(QWidget*, TripGenerationContext);
    ~TripDistributionPanel();
    void run();
    void editDeterrence();
    void editMatrices();
    void refresh();
    void translate();
    bool running() const;
    bool exportable() const;
    bool stale() const;
    std::string csv() const;
private:
    struct Data; std::unique_ptr<Data> d_;
};
}
