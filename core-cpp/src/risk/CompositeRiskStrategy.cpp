#include "omnipay/risk/CompositeRiskStrategy.hpp"
#include <numeric>
#include <chrono>

namespace omnipay {

CompositeRiskStrategy::CompositeRiskStrategy(Thresholds thresholds)
    : thresholds_(thresholds) {}

void CompositeRiskStrategy::addStrategy(std::shared_ptr<IRiskScoringStrategy> strategy, 
                                        double weight, 
                                        bool fastFailOnFraud) {
    std::lock_guard<std::mutex> lock(mutex_);
    strategies_.push_back({strategy, weight, fastFailOnFraud});
}

void CompositeRiskStrategy::clearStrategies() {
    std::lock_guard<std::mutex> lock(mutex_);
    strategies_.clear();
}

size_t CompositeRiskStrategy::getStrategyCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return strategies_.size();
}

RiskEvaluation CompositeRiskStrategy::evaluate(const TransactionContext& ctx) {
    auto startTime = std::chrono::high_resolution_clock::now();
    RiskEvaluation composite;
    composite.score = 0.0;
    composite.action = RiskAction::APPROVE;

    std::vector<StrategyEntry> activeStrategies;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        activeStrategies = strategies_;
    }

    if (activeStrategies.empty()) {
        composite.score = 5.0;
        composite.action = RiskAction::APPROVE;
        composite.evaluationNotes = "Default baseline: No active risk strategies configured";
        return composite;
    }

    double totalWeight = 0.0;
    double weightedScoreSum = 0.0;
    bool fastFailed = false;

    for (const auto& entry : activeStrategies) {
        if (!entry.strategy) continue;

        RiskEvaluation subEval = entry.strategy->evaluate(ctx);

        // Collect rule triggers
        for (const auto& rule : subEval.ruleTriggers) {
            composite.ruleTriggers.push_back(rule);
        }

        // Fast-fail circuit breaker
        if (entry.fastFailOnFraud && subEval.action == RiskAction::DECLINE_FRAUD) {
            composite.score = std::max(composite.score, subEval.score);
            composite.action = RiskAction::DECLINE_FRAUD;
            composite.evaluationNotes = "[" + entry.strategy->getStrategyName() + " FAST-FAIL] " + subEval.evaluationNotes;
            fastFailed = true;
            break;
        }

        weightedScoreSum += (subEval.score * entry.weight);
        totalWeight += entry.weight;

        if (!subEval.evaluationNotes.empty()) {
            if (!composite.evaluationNotes.empty()) {
                composite.evaluationNotes += "; ";
            }
            composite.evaluationNotes += "[" + entry.strategy->getStrategyName() + "] " + subEval.evaluationNotes;
        }
    }

    if (!fastFailed) {
        if (totalWeight > 0.0) {
            composite.score = weightedScoreSum / totalWeight;
        } else {
            composite.score = 5.0;
        }

        // Apply risk classification thresholds
        if (composite.score >= thresholds_.manualReviewBelow) {
            composite.action = RiskAction::DECLINE_FRAUD;
        } else if (composite.score >= thresholds_.challengeBelow) {
            composite.action = RiskAction::MANUAL_REVIEW;
        } else if (composite.score >= thresholds_.approveBelow) {
            composite.action = RiskAction::CHALLENGE_3DS;
        } else {
            composite.action = RiskAction::APPROVE;
        }
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    composite.evaluationMicros = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count());

    return composite;
}

} // namespace omnipay
