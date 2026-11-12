#include "omnipay/engine/TransactionEngine.hpp"
#include <algorithm>
#include <chrono>

namespace omnipay {

TransactionEngine::TransactionEngine(EngineConfig config)
    : config_(config) {
    
    // Initialize worker thread pool
    pool_ = std::make_unique<TransactionProcessorPool>(
        config_.workerThreads, config_.queueCapacity);

    // Setup default risk scoring pipeline (Composite Strategy Pattern)
    if (config_.enableDefaultRiskPipeline) {
        auto composite = std::make_shared<CompositeRiskStrategy>();
        auto velocity = std::make_shared<VelocityCheckStrategy>();
        auto geo = std::make_shared<GeoAnomalyStrategy>();

        composite->addStrategy(velocity, 0.5, true);
        composite->addStrategy(geo, 0.5, true);
        riskStrategy_ = composite;
    }

    // Setup default observers (Observer Pattern)
    if (config_.enableDefaultObservers) {
        auditObserver_ = std::make_shared<AuditLoggingObserver>();
        telemetryObserver_ = std::make_shared<LatencyTelemetryObserver>();
        alertObserver_ = std::make_shared<AlertDispatchObserver>();

        registerObserver(auditObserver_);
        registerObserver(telemetryObserver_);
        registerObserver(alertObserver_);
    }
}

TransactionEngine::~TransactionEngine() {
    shutdown();
}

void TransactionEngine::shutdown() {
    if (pool_) {
        pool_->shutdown();
    }
}

size_t TransactionEngine::getWorkerThreadCount() const {
    return pool_ ? pool_->getThreadCount() : 0;
}

size_t TransactionEngine::getPendingQueueSize() const {
    return pool_ ? pool_->getQueueSize() : 0;
}

void TransactionEngine::setRiskStrategy(std::shared_ptr<IRiskScoringStrategy> strategy) {
    riskStrategy_ = strategy;
}

std::shared_ptr<IRiskScoringStrategy> TransactionEngine::getRiskStrategy() const {
    return riskStrategy_;
}

void TransactionEngine::registerObserver(std::shared_ptr<ITransactionObserver> observer) {
    if (!observer) return;
    std::lock_guard<std::mutex> lock(observerMutex_);
    observers_.push_back(observer);
}

void TransactionEngine::unregisterObserver(std::shared_ptr<ITransactionObserver> observer) {
    std::lock_guard<std::mutex> lock(observerMutex_);
    observers_.erase(
        std::remove(observers_.begin(), observers_.end(), observer),
        observers_.end());
}

void TransactionEngine::clearObservers() {
    std::lock_guard<std::mutex> lock(observerMutex_);
    observers_.clear();
}

void TransactionEngine::notifySubmitted(const TransactionContext& ctx) {
    std::vector<std::shared_ptr<ITransactionObserver>> obsCopy;
    {
        std::lock_guard<std::mutex> lock(observerMutex_);
        obsCopy = observers_;
    }
    for (const auto& obs : obsCopy) {
        if (obs) obs->onTransactionSubmitted(ctx);
    }
}

void TransactionEngine::notifyRiskEvaluated(const TransactionContext& ctx, const RiskEvaluation& risk) {
    std::vector<std::shared_ptr<ITransactionObserver>> obsCopy;
    {
        std::lock_guard<std::mutex> lock(observerMutex_);
        obsCopy = observers_;
    }
    for (const auto& obs : obsCopy) {
        if (obs) obs->onRiskEvaluated(ctx, risk);
    }
}

void TransactionEngine::notifyCompleted(const TransactionContext& ctx, const TransactionResult& result) {
    std::vector<std::shared_ptr<ITransactionObserver>> obsCopy;
    {
        std::lock_guard<std::mutex> lock(observerMutex_);
        obsCopy = observers_;
    }
    for (const auto& obs : obsCopy) {
        if (obs) obs->onTransactionCompleted(ctx, result);
    }
}

void TransactionEngine::notifyFailed(const TransactionContext& ctx, const std::string& errCode, const std::string& message) {
    std::vector<std::shared_ptr<ITransactionObserver>> obsCopy;
    {
        std::lock_guard<std::mutex> lock(observerMutex_);
        obsCopy = observers_;
    }
    for (const auto& obs : obsCopy) {
        if (obs) obs->onTransactionFailed(ctx, errCode, message);
    }
}

TransactionResult TransactionEngine::processTransaction(const TransactionContext& ctx, 
                                                        IPaymentInstrument& instrument) {
    auto startClock = std::chrono::high_resolution_clock::now();

    // 1. Notify observers of new transaction ingest
    notifySubmitted(ctx);

    TransactionResult result;
    result.transactionId = ctx.transactionId;
    result.currency = ctx.currency;
    result.authorizedAmount = ctx.amount;
    result.maskedInstrument = instrument.getMaskedIdentifier();

    // 2. Validate instrument format and credentials
    if (!instrument.validate()) {
        result.status = TransactionStatus::FAILED;
        result.responseCode = "14";
        result.responseMessage = "Instrument validation failed";
        auto endClock = std::chrono::high_resolution_clock::now();
        result.latencyMicros = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::microseconds>(endClock - startClock).count());
        notifyFailed(ctx, result.responseCode, result.responseMessage);
        return result;
    }

    // 3. Evaluate Risk via pluggable strategy
    RiskEvaluation risk;
    if (riskStrategy_) {
        risk = riskStrategy_->evaluate(ctx);
    } else {
        risk.score = 0.0;
        risk.action = RiskAction::APPROVE;
    }
    result.riskResult = risk;
    notifyRiskEvaluated(ctx, risk);

    // 4. Process Authorization based on Risk and Instrument rules
    if (risk.action == RiskAction::DECLINE_FRAUD) {
        result.status = TransactionStatus::RISK_REJECTED;
        result.responseCode = "05";
        result.responseMessage = "Declined by Risk Strategy: " + risk.evaluationNotes;
    } else {
        AuthorizationResult authRes = instrument.authorize(ctx.amount, ctx.currency, risk);
        if (authRes.success) {
            result.status = TransactionStatus::AUTHORIZED;
            result.authorizationCode = authRes.authCode;
            result.responseCode = authRes.responseCode;
            result.responseMessage = authRes.message;
        } else {
            result.status = TransactionStatus::DECLINED;
            result.responseCode = authRes.responseCode;
            result.responseMessage = authRes.message;
        }
    }

    auto endClock = std::chrono::high_resolution_clock::now();
    result.latencyMicros = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(endClock - startClock).count());
    result.completedAt = std::chrono::system_clock::now();

    // 5. Notify observers of transaction finalization
    notifyCompleted(ctx, result);

    return result;
}

std::future<TransactionResult> TransactionEngine::processTransactionAsync(
    TransactionContext ctx, 
    std::shared_ptr<IPaymentInstrument> instrument) {
    
    return pool_->submit([this, c = std::move(ctx), inst = std::move(instrument)]() mutable {
        return this->processTransaction(c, *inst);
    });
}

std::vector<TransactionResult> TransactionEngine::processBatch(
    const std::vector<std::pair<TransactionContext, std::shared_ptr<IPaymentInstrument>>>& batch) {
    
    std::vector<std::future<TransactionResult>> futures;
    futures.reserve(batch.size());

    for (const auto& item : batch) {
        futures.push_back(processTransactionAsync(item.first, item.second));
    }

    std::vector<TransactionResult> results;
    results.reserve(futures.size());

    for (auto& fut : futures) {
        results.push_back(fut.get());
    }

    return results;
}

} // namespace omnipay
