#pragma once

#include "omnipay/types.hpp"
#include <string>

namespace omnipay {

/**
 * @brief Observer Design Pattern for event-driven telemetry, auditing, and alert dispatching.
 * Strictly adheres to GoF Behavioral Design Patterns and decoupling.
 */
class ITransactionObserver {
public:
    virtual ~ITransactionObserver() = default;

    virtual void onTransactionSubmitted(const TransactionContext& ctx) = 0;
    virtual void onRiskEvaluated(const TransactionContext& ctx, const RiskEvaluation& risk) = 0;
    virtual void onTransactionCompleted(const TransactionContext& ctx, const TransactionResult& result) = 0;
    virtual void onTransactionFailed(const TransactionContext& ctx, 
                                     const std::string& errorCode, 
                                     const std::string& message) = 0;
};

} // namespace omnipay
