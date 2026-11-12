#pragma once

#include "omnipay/observer/ITransactionObserver.hpp"
#include <mutex>
#include <vector>
#include <functional>
#include <atomic>

namespace omnipay {

struct AlertEvent {
    std::string alertType;
    std::string transactionId;
    std::string accountId;
    double riskScore;
    std::string ruleTrigger;
    std::string message;
    std::chrono::system_clock::time_point timestamp;
};

/**
 * @brief Real-time Security & Fraud Alert Dispatcher Observer.
 * Dispatches automated alerts to monitoring listeners when risk limits are breached.
 */
class AlertDispatchObserver : public ITransactionObserver {
public:
    using AlertCallback = std::function<void(const AlertEvent&)>;

    explicit AlertDispatchObserver(double alertRiskThreshold = 75.0);

    void setAlertCallback(AlertCallback callback);

    void onTransactionSubmitted(const TransactionContext& ctx) override;
    void onRiskEvaluated(const TransactionContext& ctx, const RiskEvaluation& risk) override;
    void onTransactionCompleted(const TransactionContext& ctx, const TransactionResult& result) override;
    void onTransactionFailed(const TransactionContext& ctx, 
                             const std::string& errorCode, 
                             const std::string& message) override;

    std::vector<AlertEvent> getDispatchedAlerts() const;
    size_t getAlertCount() const;
    void clearAlerts();

private:
    double alertRiskThreshold_{75.0};
    AlertCallback alertCallback_{nullptr};
    std::atomic<size_t> alertCount_{0};
    mutable std::mutex mutex_;
    std::vector<AlertEvent> dispatchedAlerts_;
};

} // namespace omnipay
