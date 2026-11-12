#pragma once

#include "omnipay/observer/ITransactionObserver.hpp"
#include <mutex>
#include <vector>
#include <string>
#include <deque>

namespace omnipay {

struct AuditLogEntry {
    std::string timestampIso;
    std::string level;
    std::string eventType;
    std::string transactionId;
    std::string accountId;
    std::string maskedIdentifier;
    double amount;
    std::string currency;
    std::string statusOrAction;
    std::string details;
    uint32_t threadId;
};

/**
 * @brief PCI-DSS and ISO 8583 Compliant Audit Logging Observer.
 * Enforces sensitive data masking and immutable chronological audit trail.
 */
class AuditLoggingObserver : public ITransactionObserver {
public:
    explicit AuditLoggingObserver(size_t maxBufferEntries = 50000);

    void onTransactionSubmitted(const TransactionContext& ctx) override;
    void onRiskEvaluated(const TransactionContext& ctx, const RiskEvaluation& risk) override;
    void onTransactionCompleted(const TransactionContext& ctx, const TransactionResult& result) override;
    void onTransactionFailed(const TransactionContext& ctx, 
                             const std::string& errorCode, 
                             const std::string& message) override;

    std::vector<AuditLogEntry> getAuditTrail(size_t maxCount = 100) const;
    size_t getEntryCount() const;
    void clear();

private:
    void recordEntry(std::string level,
                     std::string eventType,
                     const TransactionContext& ctx,
                     std::string statusOrAction,
                     std::string details);

    size_t maxBufferEntries_;
    mutable std::mutex mutex_;
    std::deque<AuditLogEntry> logs_;
};

} // namespace omnipay
