#include "omnipay/observer/AuditLoggingObserver.hpp"
#include "omnipay/instrument/IPaymentInstrument.hpp"
#include <iomanip>
#include <sstream>
#include <thread>

namespace omnipay {

static std::string formatIsoTimestamp(const std::chrono::system_clock::time_point& tp) {
    auto timeT = std::chrono::system_clock::to_time_t(tp);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch()) % 1000;
    std::tm tmBuffer;
#if defined(_WIN32)
    gmtime_s(&tmBuffer, &timeT);
#else
    gmtime_r(&timeT, &tmBuffer);
#endif
    std::stringstream ss;
    ss << std::put_time(&tmBuffer, "%Y-%m-%dT%H:%M:%S")
       << "." << std::setfill('0') << std::setw(3) << ms.count() << "Z";
    return ss.str();
}

AuditLoggingObserver::AuditLoggingObserver(size_t maxBufferEntries)
    : maxBufferEntries_(maxBufferEntries) {}

void AuditLoggingObserver::recordEntry(std::string level,
                                       std::string eventType,
                                       const TransactionContext& ctx,
                                       std::string statusOrAction,
                                       std::string details) {
    AuditLogEntry entry;
    entry.timestampIso = formatIsoTimestamp(ctx.timestamp);
    entry.level = std::move(level);
    entry.eventType = std::move(eventType);
    entry.transactionId = ctx.transactionId;
    entry.accountId = ctx.accountId;
    entry.maskedIdentifier = IPaymentInstrument::maskPan(ctx.accountId);
    entry.amount = ctx.amount;
    entry.currency = ctx.currency;
    entry.statusOrAction = std::move(statusOrAction);
    entry.details = std::move(details);
    
    // Hash thread id into 32-bit unsigned
    auto hash = std::hash<std::thread::id>{}(std::this_thread::get_id());
    entry.threadId = static_cast<uint32_t>(hash & 0xFFFFFFFF);

    std::lock_guard<std::mutex> lock(mutex_);
    if (logs_.size() >= maxBufferEntries_) {
        logs_.pop_front();
    }
    logs_.push_back(std::move(entry));
}

void AuditLoggingObserver::onTransactionSubmitted(const TransactionContext& ctx) {
    recordEntry("INFO", "TX_SUBMITTED", ctx, "INITIATED", 
                "Transaction received for " + instrumentTypeToString(ctx.instrumentType));
}

void AuditLoggingObserver::onRiskEvaluated(const TransactionContext& ctx, const RiskEvaluation& risk) {
    std::string level = (risk.action == RiskAction::DECLINE_FRAUD) ? "WARN" : "INFO";
    std::string details = "Score=" + std::to_string(risk.score) + "; Notes=" + risk.evaluationNotes;
    recordEntry(level, "RISK_EVALUATED", ctx, riskActionToString(risk.action), details);
}

void AuditLoggingObserver::onTransactionCompleted(const TransactionContext& ctx, const TransactionResult& result) {
    std::string level = (result.status == TransactionStatus::AUTHORIZED) ? "INFO" : "WARN";
    std::string details = "AuthCode=" + result.authorizationCode + 
                          "; Latency=" + std::to_string(result.latencyMicros) + "us";
    recordEntry(level, "TX_COMPLETED", ctx, transactionStatusToString(result.status), details);
}

void AuditLoggingObserver::onTransactionFailed(const TransactionContext& ctx, 
                                               const std::string& errorCode, 
                                               const std::string& message) {
    recordEntry("ERROR", "TX_FAILED", ctx, "FAILED", 
                "ErrorCode=" + errorCode + "; Message=" + message);
}

std::vector<AuditLogEntry> AuditLoggingObserver::getAuditTrail(size_t maxCount) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<AuditLogEntry> result;
    size_t count = std::min(maxCount, logs_.size());
    auto it = logs_.rbegin();
    for (size_t i = 0; i < count && it != logs_.rend(); ++i, ++it) {
        result.push_back(*it);
    }
    return result;
}

size_t AuditLoggingObserver::getEntryCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return logs_.size();
}

void AuditLoggingObserver::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    logs_.clear();
}

} // namespace omnipay
