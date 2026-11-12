#include "omnipay/observer/LatencyTelemetryObserver.hpp"
#include <algorithm>
#include <cmath>

namespace omnipay {

LatencyTelemetryObserver::LatencyTelemetryObserver(size_t maxSampleRetention)
    : maxSampleRetention_(maxSampleRetention) {
    latencySamplesMicros_.reserve(16384);
}

void LatencyTelemetryObserver::onTransactionSubmitted(const TransactionContext& ctx) {
    (void)ctx;
    totalCount_.fetch_add(1, std::memory_order_relaxed);
}

void LatencyTelemetryObserver::onRiskEvaluated(const TransactionContext& ctx, const RiskEvaluation& risk) {
    (void)ctx;
    (void)risk;
}

void LatencyTelemetryObserver::onTransactionCompleted(const TransactionContext& ctx, const TransactionResult& result) {
    (void)ctx;
    if (result.status == TransactionStatus::AUTHORIZED) {
        successCount_.fetch_add(1, std::memory_order_relaxed);
    } else {
        failCount_.fetch_add(1, std::memory_order_relaxed);
    }

    cumulativeMicros_.fetch_add(result.latencyMicros, std::memory_order_relaxed);

    std::lock_guard<std::mutex> lock(mutex_);
    if (latencySamplesMicros_.size() < maxSampleRetention_) {
        latencySamplesMicros_.push_back(result.latencyMicros);
    }
}

void LatencyTelemetryObserver::onTransactionFailed(const TransactionContext& ctx, 
                                                   const std::string& errorCode, 
                                                   const std::string& message) {
    (void)ctx;
    (void)errorCode;
    (void)message;
    failCount_.fetch_add(1, std::memory_order_relaxed);
}

LatencyStats LatencyTelemetryObserver::getSnapshot() const {
    LatencyStats stats;
    stats.totalTransactions = totalCount_.load(std::memory_order_relaxed);
    stats.successfulTransactions = successCount_.load(std::memory_order_relaxed);
    stats.failedTransactions = failCount_.load(std::memory_order_relaxed);

    std::vector<uint64_t> samples;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        samples = latencySamplesMicros_;
    }

    if (samples.empty()) {
        return stats;
    }

    std::sort(samples.begin(), samples.end());

    double sumMs = 0.0;
    for (uint64_t s : samples) {
        sumMs += (static_cast<double>(s) / 1000.0);
    }
    stats.averageLatencyMs = sumMs / static_cast<double>(samples.size());
    stats.minLatencyMs = static_cast<double>(samples.front()) / 1000.0;
    stats.maxLatencyMs = static_cast<double>(samples.back()) / 1000.0;

    auto getPercentile = [&samples](double pct) -> double {
        if (samples.empty()) return 0.0;
        size_t idx = static_cast<size_t>(std::ceil((pct / 100.0) * static_cast<double>(samples.size()))) - 1;
        if (idx >= samples.size()) idx = samples.size() - 1;
        return static_cast<double>(samples[idx]) / 1000.0;
    };

    stats.p50LatencyMs = getPercentile(50.0);
    stats.p90LatencyMs = getPercentile(90.0);
    stats.p95LatencyMs = getPercentile(95.0);
    stats.p99LatencyMs = getPercentile(99.0);

    // Compute sample standard deviation
    double varSum = 0.0;
    for (uint64_t s : samples) {
        double diff = (static_cast<double>(s) / 1000.0) - stats.averageLatencyMs;
        varSum += (diff * diff);
    }
    stats.standardDeviationMs = std::sqrt(varSum / static_cast<double>(samples.size()));

    return stats;
}

void LatencyTelemetryObserver::reset() {
    totalCount_.store(0, std::memory_order_relaxed);
    successCount_.store(0, std::memory_order_relaxed);
    failCount_.store(0, std::memory_order_relaxed);
    cumulativeMicros_.store(0, std::memory_order_relaxed);

    std::lock_guard<std::mutex> lock(mutex_);
    latencySamplesMicros_.clear();
}

} // namespace omnipay
