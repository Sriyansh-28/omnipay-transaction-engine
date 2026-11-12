#include "omnipay/observer/AlertDispatchObserver.hpp"

namespace omnipay {

AlertDispatchObserver::AlertDispatchObserver(double alertRiskThreshold)
    : alertRiskThreshold_(alertRiskThreshold) {}

void AlertDispatchObserver::setAlertCallback(AlertCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    alertCallback_ = std::move(callback);
}

void AlertDispatchObserver::onTransactionSubmitted(const TransactionContext& ctx) {
    (void)ctx;
}

void AlertDispatchObserver::onRiskEvaluated(const TransactionContext& ctx, const RiskEvaluation& risk) {
    if (risk.score >= alertRiskThreshold_ || risk.action == RiskAction::DECLINE_FRAUD) {
        AlertEvent alert;
        alert.alertType = (risk.action == RiskAction::DECLINE_FRAUD) ? "CRITICAL_FRAUD" : "ELEVATED_RISK";
        alert.transactionId = ctx.transactionId;
        alert.accountId = ctx.accountId;
        alert.riskScore = risk.score;
        alert.ruleTrigger = risk.ruleTriggers.empty() ? "MANUAL_THRESHOLD" : risk.ruleTriggers.front();
        alert.message = risk.evaluationNotes;
        alert.timestamp = std::chrono::system_clock::now();

        alertCount_.fetch_add(1, std::memory_order_relaxed);

        AlertCallback cbCopy = nullptr;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            dispatchedAlerts_.push_back(alert);
            cbCopy = alertCallback_;
        }

        if (cbCopy) {
            cbCopy(alert);
        }
    }
}

void AlertDispatchObserver::onTransactionCompleted(const TransactionContext& ctx, const TransactionResult& result) {
    (void)ctx;
    (void)result;
}

void AlertDispatchObserver::onTransactionFailed(const TransactionContext& ctx, 
                                               const std::string& errorCode, 
                                               const std::string& message) {
    AlertEvent alert;
    alert.alertType = "TRANSACTION_FAILURE";
    alert.transactionId = ctx.transactionId;
    alert.accountId = ctx.accountId;
    alert.riskScore = 0.0;
    alert.ruleTrigger = errorCode;
    alert.message = message;
    alert.timestamp = std::chrono::system_clock::now();

    alertCount_.fetch_add(1, std::memory_order_relaxed);

    AlertCallback cbCopy = nullptr;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        dispatchedAlerts_.push_back(alert);
        cbCopy = alertCallback_;
    }

    if (cbCopy) {
        cbCopy(alert);
    }
}

std::vector<AlertEvent> AlertDispatchObserver::getDispatchedAlerts() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return dispatchedAlerts_;
}

size_t AlertDispatchObserver::getAlertCount() const {
    return alertCount_.load(std::memory_order_relaxed);
}

void AlertDispatchObserver::clearAlerts() {
    std::lock_guard<std::mutex> lock(mutex_);
    dispatchedAlerts_.clear();
    alertCount_.store(0, std::memory_order_relaxed);
}

} // namespace omnipay
