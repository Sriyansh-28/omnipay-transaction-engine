#include <iostream>
#include <cassert>
#include <chrono>
#include <thread>
#include <vector>
#include <string>

#include "omnipay/types.hpp"
#include "omnipay/instrument/PaymentInstrumentFactory.hpp"
#include "omnipay/risk/VelocityCheckStrategy.hpp"
#include "omnipay/risk/GeoAnomalyStrategy.hpp"
#include "omnipay/risk/CompositeRiskStrategy.hpp"
#include "omnipay/observer/AuditLoggingObserver.hpp"
#include "omnipay/observer/LatencyTelemetryObserver.hpp"
#include "omnipay/observer/AlertDispatchObserver.hpp"
#include "omnipay/concurrency/ConcurrentQueue.hpp"
#include "omnipay/concurrency/TransactionProcessorPool.hpp"
#include "omnipay/engine/TransactionEngine.hpp"

using namespace omnipay;

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "[FAIL] Line " << __LINE__ << ": " << msg << std::endl; \
            std::exit(1); \
        } \
    } while (0)

void testPaymentInstrumentsAndFactory() {
    std::cout << ">>> Running Unit Test: Factory Pattern & Polymorphic Instruments..." << std::endl;

    // 1. Valid Credit Card (Visa)
    auto visa = PaymentInstrumentFactory::createCreditCard(
        "4111111111111111", "Jane Doe", 11, 2028, "456", "94105", true);
    TEST_ASSERT(visa->validate() == true, "Visa card should be valid");
    TEST_ASSERT(visa->getType() == InstrumentType::CREDIT_CARD, "Type should be CREDIT_CARD");
    TEST_ASSERT(visa->getBrand() == "VISA", "Brand should be VISA");
    TEST_ASSERT(visa->getMaskedIdentifier().find("411111") == 0, "Masked PAN must start with BIN 411111");

    // 2. Invalid Credit Card (Bad Luhn)
    auto badCard = PaymentInstrumentFactory::createCreditCard(
        "4111111111111112", "Jane Doe", 11, 2028, "456");
    TEST_ASSERT(badCard->validate() == false, "Invalid Luhn card should fail validation");

    // 3. Debit Card with Balance & Limit checks
    auto debit = PaymentInstrumentFactory::createDebitCard(
        "4111111111111111", "John Smith", 8, 2029, "PIN_TOKEN_999",
        DebitAccountType::CHECKING, 1000.0, 500.0);
    TEST_ASSERT(debit->validate() == true, "Debit card should validate");
    RiskEvaluation lowRisk{5.0, RiskAction::APPROVE, {}, "Normal", 10};
    
    // Auth within limits
    auto debitAuth1 = debit->authorize(200.0, "USD", lowRisk);
    TEST_ASSERT(debitAuth1.success == true, "Debit authorization should succeed under limit");

    // Auth exceeding daily limit ($500 limit, requesting $600)
    auto debitAuth2 = debit->authorize(600.0, "USD", lowRisk);
    TEST_ASSERT(debitAuth2.success == false, "Debit authorization should fail when exceeding limit");
    TEST_ASSERT(debitAuth2.responseCode == "61", "Exceeds daily limit response code expected");

    // 4. NFC Token (Apple Pay)
    auto nfc = PaymentInstrumentFactory::createNFCToken(
        "4888888888888888", "40010075050", "DUMMY_ARQC_CRYPTOGRAM", 10,
        NFCPayProvider::APPLE_PAY, true, 60);
    TEST_ASSERT(nfc->validate() == true, "NFC Token should validate");
    TEST_ASSERT(nfc->getBrand() == "APPLE_PAY", "NFC Provider should match Apple Pay");
    auto nfcAuth = nfc->authorize(50.0, "USD", lowRisk);
    TEST_ASSERT(nfcAuth.success == true, "NFC auth should succeed");

    // 5. QR Wallet (EMVCo Dynamic)
    auto qr = PaymentInstrumentFactory::createQRWallet(
        "00020101021226480014com.omnipay.qr0110TESTMERCH520459995303840540510.005802US5910TEST STORE6007SF6304ABCD",
        "USER_WALLET_77", QRWalletProvider::ALIPAY, true, 300, "SIG_VALID");
    TEST_ASSERT(qr->validate() == true, "QR Wallet should validate");
    TEST_ASSERT(qr->getBrand() == "ALIPAY", "QR Wallet brand should be ALIPAY");
    auto qrAuth = qr->authorize(15.0, "USD", lowRisk);
    TEST_ASSERT(qrAuth.success == true, "QR authorization should succeed");

    std::cout << "[PASS] Payment Instruments & Factory Pattern verified." << std::endl;
}

void testRiskScoringStrategies() {
    std::cout << ">>> Running Unit Test: Strategy Pattern (Velocity, GeoAnomaly, Composite)..." << std::endl;

    // 1. Velocity Strategy
    VelocityCheckStrategy velocity;
    TransactionContext ctxBase;
    ctxBase.transactionId = "TX-VEL-1";
    ctxBase.accountId = "ACC-VEL-100";
    ctxBase.amount = 100.0;
    ctxBase.timestamp = std::chrono::system_clock::now();

    // Submit 4 transactions in quick succession (should be allowed / challenge)
    for (int i = 0; i < 4; ++i) {
        ctxBase.transactionId = "TX-VEL-" + std::to_string(i);
        auto eval = velocity.evaluate(ctxBase);
        TEST_ASSERT(eval.action != RiskAction::DECLINE_FRAUD, "Under-limit transactions should not be declined for fraud");
    }

    // 5th transaction triggers velocity burst breach (> 5 in 60s)
    ctxBase.transactionId = "TX-VEL-5";
    auto burstEval = velocity.evaluate(ctxBase);
    TEST_ASSERT(burstEval.action == RiskAction::DECLINE_FRAUD, "Burst frequency breach should trigger DECLINE_FRAUD");
    TEST_ASSERT(burstEval.score >= 80.0, "Velocity score should be elevated");

    // 2. GeoAnomaly Strategy (Impossible Travel)
    GeoAnomalyStrategy geo;
    TransactionContext geoCtx1;
    geoCtx1.transactionId = "TX-GEO-1";
    geoCtx1.accountId = "ACC-TRAVELER-1";
    geoCtx1.location = {37.7749, -122.4194, "US", "San Francisco"}; // SF
    geoCtx1.timestamp = std::chrono::system_clock::now();
    auto geoEval1 = geo.evaluate(geoCtx1);
    TEST_ASSERT(geoEval1.action == RiskAction::APPROVE, "Initial transaction in US should be approved");

    // Second transaction 5 minutes later in London (~8,600 km away!)
    TransactionContext geoCtx2;
    geoCtx2.transactionId = "TX-GEO-2";
    geoCtx2.accountId = "ACC-TRAVELER-1";
    geoCtx2.location = {51.5074, -0.1278, "GB", "London"}; // London
    geoCtx2.timestamp = geoCtx1.timestamp + std::chrono::minutes(5); // 5 min later
    auto geoEval2 = geo.evaluate(geoCtx2);
    TEST_ASSERT(geoEval2.action == RiskAction::DECLINE_FRAUD, "Impossible travel speed (>800 km/h) must be DECLINE_FRAUD");
    TEST_ASSERT(geoEval2.score >= 90.0, "Impossible travel risk score should be >= 90.0");

    // 3. Composite Risk Strategy
    auto composite = std::make_shared<CompositeRiskStrategy>();
    composite->addStrategy(std::make_shared<VelocityCheckStrategy>(), 0.5, true);
    composite->addStrategy(std::make_shared<GeoAnomalyStrategy>(), 0.5, true);

    TransactionContext normalCtx;
    normalCtx.transactionId = "TX-NORM-1";
    normalCtx.accountId = "ACC-NORMAL-99";
    normalCtx.amount = 25.0;
    normalCtx.location = {37.7749, -122.4194, "US", "San Francisco"};
    normalCtx.timestamp = std::chrono::system_clock::now();

    auto compEval = composite->evaluate(normalCtx);
    TEST_ASSERT(compEval.action == RiskAction::APPROVE, "Normal baseline transaction must be approved");
    TEST_ASSERT(compEval.score < 30.0, "Score should be well below approval threshold");

    std::cout << "[PASS] Strategy Pattern & Risk Engines verified." << std::endl;
}

void testObserverPattern() {
    std::cout << ">>> Running Unit Test: Observer Pattern (Audit, Latency, Alert Dispatch)..." << std::endl;

    auto audit = std::make_shared<AuditLoggingObserver>();
    auto telemetry = std::make_shared<LatencyTelemetryObserver>();
    auto alert = std::make_shared<AlertDispatchObserver>(70.0);

    bool alertFired = false;
    alert->setAlertCallback([&alertFired](const AlertEvent& ev) {
        alertFired = true;
        TEST_ASSERT(ev.riskScore >= 70.0, "Alert callback must have elevated risk score");
    });

    TransactionEngine::EngineConfig cfg;
    cfg.workerThreads = 2;
    cfg.enableDefaultObservers = false; // Custom observer registration
    TransactionEngine engine(cfg);

    engine.registerObserver(audit);
    engine.registerObserver(telemetry);
    engine.registerObserver(alert);

    auto card = PaymentInstrumentFactory::createCreditCard(
        "4111111111111111", "Audit Tester", 12, 2028, "123", "94105", true);

    TransactionContext ctx;
    ctx.transactionId = "TX-OBS-101";
    ctx.accountId = "ACC-OBS-USER";
    ctx.amount = 75.0;
    ctx.location = {37.7749, -122.4194, "US", "SF"};
    ctx.timestamp = std::chrono::system_clock::now();

    auto res = engine.processTransaction(ctx, *card);
    TEST_ASSERT(res.status == TransactionStatus::AUTHORIZED, "Transaction should be authorized");

    TEST_ASSERT(audit->getEntryCount() >= 3, "Audit trail should have logged ingest, risk, and completion");
    auto stats = telemetry->getSnapshot();
    TEST_ASSERT(stats.totalTransactions == 1, "Telemetry total transactions count must be 1");
    TEST_ASSERT(stats.successfulTransactions == 1, "Telemetry success count must be 1");
    TEST_ASSERT(stats.averageLatencyMs > 0.0, "Average latency should be positive");

    std::cout << "[PASS] Observer Pattern verified." << std::endl;
}

void testConcurrencyAndProcessorPool() {
    std::cout << ">>> Running Unit Test: Multithreading & Concurrent Processor Pool..." << std::endl;

    TransactionProcessorPool pool(4, 5000);
    std::atomic<int> counter{0};
    constexpr int totalTasks = 1000;

    std::vector<std::future<int>> futures;
    futures.reserve(totalTasks);

    for (int i = 0; i < totalTasks; ++i) {
        futures.push_back(pool.submit([&counter, i]() {
            counter.fetch_add(1, std::memory_order_relaxed);
            return i * 2;
        }));
    }

    for (int i = 0; i < totalTasks; ++i) {
        int res = futures[i].get();
        TEST_ASSERT(res == i * 2, "Future result must match returned computation");
    }

    TEST_ASSERT(counter.load() == totalTasks, "Atomic counter must reflect all executed tasks");
    pool.shutdown();
    TEST_ASSERT(pool.isRunning() == false, "Pool should report stopped after shutdown");

    std::cout << "[PASS] Concurrency & Thread Pool verified." << std::endl;
}

int main() {
    std::cout << "============================================================" << std::endl;
    std::cout << "OmniPay C++17 Core Engine: Comprehensive Unit Test Suite" << std::endl;
    std::cout << "============================================================" << std::endl;

    testPaymentInstrumentsAndFactory();
    testRiskScoringStrategies();
    testObserverPattern();
    testConcurrencyAndProcessorPool();

    std::cout << "\n============================================================" << std::endl;
    std::cout << "ALL OMNIPAY UNIT TESTS PASSED WITH 100% SUCCESS!" << std::endl;
    std::cout << "============================================================" << std::endl;
    return 0;
}
