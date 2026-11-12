#pragma once

#include "omnipay/risk/IRiskScoringStrategy.hpp"
#include <mutex>
#include <unordered_map>

namespace omnipay {

/**
 * @brief Geographical Anomaly & Impossible Travel Strategy
 * Evaluates spatial-temporal jumps between consecutive transactions.
 */
class GeoAnomalyStrategy : public IRiskScoringStrategy {
public:
    struct GeoConfig {
        double maxFeasibleSpeedKmH{800.0};  // Maximum plausible travel speed (commercial jet)
        double immediateJumpToleranceKm{15.0}; // Local radius without velocity penalty
        std::string homeCountry{"US"};
    };

    explicit GeoAnomalyStrategy(GeoConfig config = GeoConfig{});

    RiskEvaluation evaluate(const TransactionContext& ctx) override;
    std::string getStrategyName() const override { return "GeoAnomalyStrategy"; }

    static double calculateHaversineDistanceKm(double lat1, double lon1, double lat2, double lon2);

    void resetHistory();

private:
    struct LastLocationRecord {
        GeoLocation location;
        std::chrono::system_clock::time_point timestamp;
    };

    GeoConfig config_;
    mutable std::mutex mutex_;
    std::unordered_map<std::string, LastLocationRecord> lastSeenLocation_;
};

} // namespace omnipay
