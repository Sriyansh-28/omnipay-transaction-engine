#include "omnipay/risk/VelocityCheckStrategy.hpp"
#include <algorithm>
#include <chrono>

namespace omnipay {

VelocityCheckStrategy::VelocityCheckStrategy(WindowConfig config)
    : config_(config) {}

RiskEvaluation VelocityCheckStrategy::evaluate(const TransactionContext& ctx) {
    auto startTime = std::chrono::high_resolution_clock::now();
    RiskEvaluation eval;
    eval.score = 5.0; // Baseline low risk score
    eval.action = RiskAction::APPROVE;

    std::lock_guard<std::mutex> lock(mutex_);

    auto& history = accountHistory_[ctx.accountId];
    auto now = ctx.timestamp;

    // Prune entries older than the long window (10 minutes)
    while (!history.empty()) {
        auto ageSec = std::chrono::duration_cast<std::chrono::seconds>(now - history.front().timestamp).count();
        if (ageSec > static_cast<int64_t>(config_.longWindowSeconds)) {
            history.pop_front();
        } else {
            break;
        }
    }

    // Calculate metrics in short burst window and long window
    uint32_t shortBurstCount = 0;
    double longWindowVolume = 0.0;

    for (const auto& item : history) {
        auto ageSec = std::chrono::duration_cast<std::chrono::seconds>(now - item.timestamp).count();
        if (ageSec <= static_cast<int64_t>(config_.shortBurstWindowSeconds)) {
            shortBurstCount++;
        }
        longWindowVolume += item.amount;
    }

    // Check velocity conditions including incoming transaction
    uint32_t totalBurstCount = shortBurstCount + 1;
    if (totalBurstCount >= config_.maxTransactionsPerMinute) {
        eval.score = 88.0;
        eval.action = RiskAction::DECLINE_FRAUD;
        eval.ruleTriggers.push_back("RULE_VELOCITY_RAPID_BURST_BREACH");
        eval.evaluationNotes = "High frequency transaction burst: " + std::to_string(totalBurstCount) +
                               " attempts in " + std::to_string(config_.shortBurstWindowSeconds) + "s";
    } else if (totalBurstCount >= config_.maxTransactionsPerMinute - 1) {
        eval.score = 55.0;
        eval.action = RiskAction::CHALLENGE_3DS;
        eval.ruleTriggers.push_back("RULE_VELOCITY_ELEVATED_FREQUENCY");
        eval.evaluationNotes = "Elevated frequency: " + std::to_string(totalBurstCount) + " attempts detected";
    }

    if (longWindowVolume + ctx.amount > config_.maxVolumePerTenMinutes) {
        eval.score = std::max(eval.score, 78.0);
        if (eval.action != RiskAction::DECLINE_FRAUD) {
            eval.action = RiskAction::MANUAL_REVIEW;
        }
        eval.ruleTriggers.push_back("RULE_VELOCITY_VOLUME_LIMIT_EXCEEDED");
        eval.evaluationNotes += (eval.evaluationNotes.empty() ? "" : "; ") + 
                                std::string("Cumulative 10-minute volume exceeds threshold ($") +
                                std::to_string(longWindowVolume + ctx.amount) + ")";
    }

    // Record this transaction
    history.push_back({now, ctx.amount});

    auto endTime = std::chrono::high_resolution_clock::now();
    eval.evaluationMicros = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count());

    return eval;
}

void VelocityCheckStrategy::resetHistory() {
    std::lock_guard<std::mutex> lock(mutex_);
    accountHistory_.clear();
}

} // namespace omnipay
