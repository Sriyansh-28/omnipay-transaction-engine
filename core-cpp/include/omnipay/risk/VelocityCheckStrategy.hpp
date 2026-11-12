#pragma once

#include "omnipay/risk/IRiskScoringStrategy.hpp"
#include <mutex>
#include <unordered_map>
#include <deque>

namespace omnipay {

/**
 * @brief Real-time Velocity Check Strategy
 * Detects rapid burst transactions and anomalous card-testing patterns using sliding windows.
 */
class VelocityCheckStrategy : public IRiskScoringStrategy {
public:
    struct WindowConfig {
        uint32_t maxTransactionsPerMinute{5};
        double maxVolumePerTenMinutes{3000.0};
        uint32_t shortBurstWindowSeconds{60};
        uint32_t longWindowSeconds{600};
    };

    explicit VelocityCheckStrategy(WindowConfig config = WindowConfig{});

    RiskEvaluation evaluate(const TransactionContext& ctx) override;
    std::string getStrategyName() const override { return "VelocityCheckStrategy"; }

    void resetHistory();

private:
    struct TxRecord {
        std::chrono::system_clock::time_point timestamp;
        double amount;
    };

    WindowConfig config_;
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::deque<TxRecord>> accountHistory_;
};

} // namespace omnipay
