#pragma once

#include "omnipay/observer/ITransactionObserver.hpp"
#include <mutex>
#include <vector>
#include <atomic>
#include <cstdint>

namespace omnipay {

struct LatencyStats {
    uint64_t totalTransactions{0};
    uint64_t successfulTransactions{0};
    uint64_t failedTransactions{0};
    double averageLatencyMs{0.0};
    double p50LatencyMs{0.0};
    double p90LatencyMs{0.0};
    double p95LatencyMs{0.0};
    double p99LatencyMs{0.0};
    double minLatencyMs{0.0};
    double maxLatencyMs{0.0};
    double standardDeviationMs{0.0};
};

/**
 * @brief Ultra-low latency telemetry observer for real-time SLA and percentile monitoring.
 */
class LatencyTelemetryObserver : public ITransactionObserver {
public:
    LatencyTelemetryObserver(size_t maxSampleRetention = 100000);

    void onTransactionSubmitted(const TransactionContext& ctx) override;
    void onRiskEvaluated(const TransactionContext& ctx, const RiskEvaluation& risk) override;
    void onTransactionCompleted(const TransactionContext& ctx, const TransactionResult& result) override;
    void onTransactionFailed(const TransactionContext& ctx, 
                             const std::string& errorCode, 
                             const std::string& message) override;

    LatencyStats getSnapshot() const;
    void reset();

private:
    size_t maxSampleRetention_;
    std::atomic<uint64_t> totalCount_{0};
    std::atomic<uint64_t> successCount_{0};
    std::atomic<uint64_t> failCount_{0};
    std::atomic<uint64_t> cumulativeMicros_{0};

    mutable std::mutex mutex_;
    std::vector<uint64_t> latencySamplesMicros_;
};

} // namespace omnipay
