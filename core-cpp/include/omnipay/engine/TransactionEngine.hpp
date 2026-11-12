#pragma once

#include "omnipay/types.hpp"
#include "omnipay/instrument/IPaymentInstrument.hpp"
#include "omnipay/instrument/PaymentInstrumentFactory.hpp"
#include "omnipay/risk/IRiskScoringStrategy.hpp"
#include "omnipay/risk/CompositeRiskStrategy.hpp"
#include "omnipay/risk/VelocityCheckStrategy.hpp"
#include "omnipay/risk/GeoAnomalyStrategy.hpp"
#include "omnipay/observer/ITransactionObserver.hpp"
#include "omnipay/observer/AuditLoggingObserver.hpp"
#include "omnipay/observer/LatencyTelemetryObserver.hpp"
#include "omnipay/observer/AlertDispatchObserver.hpp"
#include "omnipay/concurrency/TransactionProcessorPool.hpp"
#include <memory>
#include <vector>
#include <mutex>
#include <future>

namespace omnipay {

/**
 * @brief Enterprise Payment Processing Engine.
 * Orchestrates Object-Oriented Design Patterns:
 *  - Factory Pattern (Payment Instruments)
 *  - Strategy Pattern (Fraud & Risk Scoring)
 *  - Observer Pattern (Audit, Latency Telemetry, Security Alerts)
 *  - Multithreaded Processor Pool (Concurrent Worker Execution)
 */
class TransactionEngine {
public:
    struct EngineConfig {
        size_t workerThreads{0}; // 0 = hardware concurrency
        size_t queueCapacity{100000};
        bool enableDefaultObservers{true};
        bool enableDefaultRiskPipeline{true};
    };

    explicit TransactionEngine(EngineConfig config = EngineConfig{});
    ~TransactionEngine();

    // Disable copy
    TransactionEngine(const TransactionEngine&) = delete;
    TransactionEngine& operator=(const TransactionEngine&) = delete;

    /**
     * @brief Process a transaction synchronously on caller thread
     */
    TransactionResult processTransaction(const TransactionContext& ctx, 
                                         IPaymentInstrument& instrument);

    /**
     * @brief Process a transaction asynchronously on worker pool thread
     */
    std::future<TransactionResult> processTransactionAsync(
        TransactionContext ctx, 
        std::shared_ptr<IPaymentInstrument> instrument);

    /**
     * @brief Process a batch of transactions concurrently
     */
    std::vector<TransactionResult> processBatch(
        const std::vector<std::pair<TransactionContext, std::shared_ptr<IPaymentInstrument>>>& batch);

    // Strategy Configuration (Strategy Pattern)
    void setRiskStrategy(std::shared_ptr<IRiskScoringStrategy> strategy);
    std::shared_ptr<IRiskScoringStrategy> getRiskStrategy() const;

    // Observer Registry (Observer Pattern)
    void registerObserver(std::shared_ptr<ITransactionObserver> observer);
    void unregisterObserver(std::shared_ptr<ITransactionObserver> observer);
    void clearObservers();

    // Built-in observer accessors
    std::shared_ptr<AuditLoggingObserver> getAuditObserver() const { return auditObserver_; }
    std::shared_ptr<LatencyTelemetryObserver> getTelemetryObserver() const { return telemetryObserver_; }
    std::shared_ptr<AlertDispatchObserver> getAlertObserver() const { return alertObserver_; }

    // Concurrency stats
    size_t getWorkerThreadCount() const;
    size_t getPendingQueueSize() const;
    void shutdown();

private:
    void notifySubmitted(const TransactionContext& ctx);
    void notifyRiskEvaluated(const TransactionContext& ctx, const RiskEvaluation& risk);
    void notifyCompleted(const TransactionContext& ctx, const TransactionResult& result);
    void notifyFailed(const TransactionContext& ctx, const std::string& errCode, const std::string& message);

    EngineConfig config_;
    std::unique_ptr<TransactionProcessorPool> pool_;
    std::shared_ptr<IRiskScoringStrategy> riskStrategy_;

    // Built-in standard observers
    std::shared_ptr<AuditLoggingObserver> auditObserver_;
    std::shared_ptr<LatencyTelemetryObserver> telemetryObserver_;
    std::shared_ptr<AlertDispatchObserver> alertObserver_;

    mutable std::mutex observerMutex_;
    std::vector<std::shared_ptr<ITransactionObserver>> observers_;
};

} // namespace omnipay
