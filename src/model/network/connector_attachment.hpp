#pragma once
#include "network.hpp"
#include <cmath>
namespace multra {
inline constexpr double connectorAttachmentTolerance=0.01; // metres; preserve the authoring acceptance rule
struct ConnectorAttachmentMeasurement {
    Point expected, actual;
    double distance{};
};
inline ConnectorAttachmentMeasurement measureConnectorAttachment(const Network& network,
    const LaneReference& reference,bool outgoing,Point actual) {
    const auto expected=laneAttachment(network,reference,outgoing);
    return {expected,actual,std::hypot(actual.x-expected.x,actual.y-expected.y)};
}
}
