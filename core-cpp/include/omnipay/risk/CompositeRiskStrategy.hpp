#pragma once

#include "omnipay/risk/IRiskScoringStrategy.hpp"
#include <vector>
#include <memory>
#include <mutex>

namespace omnipay {

/**
 * @brief Composite Strategy Pattern for multi-tier risk scoring.
 * Aggregates pluggable algorithms with dynamic weights and fast-fail circuit breaking.
 */
class CompositeRiskStrategy : public IRiskScoringStrategy {
public:
    struct StrategyEntry {
        std::shared_ptr<IRiskScoringStrategy> strategy;
        double weight{1.0};
        bool fastFailOnFraud{true};
    };

    struct Thresholds {
        double approveBelow{30.0};
        double challengeBelow{70.0};
        double manualReviewBelow{85.0};
    };

    explicit CompositeRiskStrategy(Thresholds thresholds = Thresholds{});

    void addStrategy(std::shared_ptr<IRiskScoringStrategy> strategy, 
                     double weight = 1.0, 
                     bool fastFailOnFraud = true);

    void clearStrategies();

    RiskEvaluation evaluate(const TransactionContext& ctx) override;
    std::string getStrategyName() const override { return "CompositeRiskStrategy"; }

    size_t getStrategyCount() const;

private:
    std::vector<StrategyEntry> strategies_;
    Thresholds thresholds_;
    mutable std::mutex mutex_;
};

} // namespace omnipay
