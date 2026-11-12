#include "omnipay/risk/GeoAnomalyStrategy.hpp"
#include <cmath>
#include <chrono>

namespace omnipay {

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static double toRadians(double deg) {
    return deg * (M_PI / 180.0);
}

GeoAnomalyStrategy::GeoAnomalyStrategy(GeoConfig config)
    : config_(config) {}

double GeoAnomalyStrategy::calculateHaversineDistanceKm(double lat1, double lon1, double lat2, double lon2) {
    constexpr double earthRadiusKm = 6371.0;

    double dLat = toRadians(lat2 - lat1);
    double dLon = toRadians(lon2 - lon1);

    double a = std::sin(dLat / 2.0) * std::sin(dLat / 2.0) +
               std::cos(toRadians(lat1)) * std::cos(toRadians(lat2)) *
               std::sin(dLon / 2.0) * std::sin(dLon / 2.0);

    double c = 2.0 * std::atan2(std::sqrt(a), std::sqrt(1.0 - a));
    return earthRadiusKm * c;
}

RiskEvaluation GeoAnomalyStrategy::evaluate(const TransactionContext& ctx) {
    auto startTime = std::chrono::high_resolution_clock::now();
    RiskEvaluation eval;
    eval.score = 5.0; // Baseline low risk score
    eval.action = RiskAction::APPROVE;

    std::lock_guard<std::mutex> lock(mutex_);

    auto it = lastSeenLocation_.find(ctx.accountId);
    if (it != lastSeenLocation_.end()) {
        const auto& prev = it->second;
        double distKm = calculateHaversineDistanceKm(
            prev.location.latitude, prev.location.longitude,
            ctx.location.latitude, ctx.location.longitude);

        auto elapsedSec = std::chrono::duration_cast<std::chrono::seconds>(ctx.timestamp - prev.timestamp).count();
        if (elapsedSec < 1) elapsedSec = 1; // Prevent div by zero

        double elapsedHours = static_cast<double>(elapsedSec) / 3600.0;
        double speedKmH = distKm / elapsedHours;

        if (distKm > config_.immediateJumpToleranceKm) {
            if (speedKmH > config_.maxFeasibleSpeedKmH) {
                // Impossible travel speed (e.g. SF to London in 10 minutes)
                eval.score = 95.0;
                eval.action = RiskAction::DECLINE_FRAUD;
                eval.ruleTriggers.push_back("RULE_GEO_IMPOSSIBLE_TRAVEL");
                eval.evaluationNotes = "Impossible travel velocity detected: " +
                                       std::to_string(static_cast<int>(speedKmH)) + " km/h over " +
                                       std::to_string(static_cast<int>(distKm)) + " km";
            } else if (speedKmH > config_.maxFeasibleSpeedKmH * 0.7) {
                // Borderline fast transit
                eval.score = 60.0;
                eval.action = RiskAction::CHALLENGE_3DS;
                eval.ruleTriggers.push_back("RULE_GEO_RAPID_DISPLACEMENT");
                eval.evaluationNotes = "High displacement velocity: " +
                                       std::to_string(static_cast<int>(speedKmH)) + " km/h";
            }
        }
    } else {
        // First time seeing this account
        if (!ctx.location.countryCode.empty() && ctx.location.countryCode != config_.homeCountry) {
            eval.score = 40.0;
            eval.action = RiskAction::CHALLENGE_3DS;
            eval.ruleTriggers.push_back("RULE_GEO_CROSS_BORDER_ORIGIN");
            eval.evaluationNotes = "Initial transaction originates outside home country (" +
                                   ctx.location.countryCode + " vs " + config_.homeCountry + ")";
        }
    }

    // Update location history for this account
    lastSeenLocation_[ctx.accountId] = {ctx.location, ctx.timestamp};

    auto endTime = std::chrono::high_resolution_clock::now();
    eval.evaluationMicros = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count());

    return eval;
}

void GeoAnomalyStrategy::resetHistory() {
    std::lock_guard<std::mutex> lock(mutex_);
    lastSeenLocation_.clear();
}

} // namespace omnipay
