#pragma once

#include "omnipay/types.hpp"
#include <string>
#include <memory>

namespace omnipay {

/**
 * @brief Strategy Design Pattern for pluggable fraud and risk scoring.
 * Allows runtime composition and hot-swapping of verification algorithms.
 */
class IRiskScoringStrategy {
public:
    virtual ~IRiskScoringStrategy() = default;

    /**
     * @brief Evaluate transaction context and produce a risk evaluation
     */
    virtual RiskEvaluation evaluate(const TransactionContext& ctx) = 0;

    /**
     * @brief Human-readable name of the strategy algorithm
     */
    virtual std::string getStrategyName() const = 0;
};

} // namespace omnipay
