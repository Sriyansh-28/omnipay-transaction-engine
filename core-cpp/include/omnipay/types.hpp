#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <cstdint>
#include <sstream>
#include <iomanip>

namespace omnipay {

/**
 * @brief Supported Payment Instrument Types
 */
enum class InstrumentType {
    CREDIT_CARD,
    DEBIT_CARD,
    NFC_TOKEN,
    QR_WALLET
};

inline std::string instrumentTypeToString(InstrumentType type) {
    switch (type) {
        case InstrumentType::CREDIT_CARD: return "CREDIT_CARD";
        case InstrumentType::DEBIT_CARD:  return "DEBIT_CARD";
        case InstrumentType::NFC_TOKEN:   return "NFC_TOKEN";
        case InstrumentType::QR_WALLET:   return "QR_WALLET";
        default:                          return "UNKNOWN";
    }
}

/**
 * @brief Transaction lifecycle states
 */
enum class TransactionStatus {
    INITIATED,
    RISK_PENDING,
    RISK_APPROVED,
    RISK_CHALLENGED,
    RISK_REJECTED,
    AUTHORIZED,
    CAPTURED,
    SETTLED,
    DECLINED,
    REFUNDED,
    FAILED
};

inline std::string transactionStatusToString(TransactionStatus status) {
    switch (status) {
        case TransactionStatus::INITIATED:       return "INITIATED";
        case TransactionStatus::RISK_PENDING:    return "RISK_PENDING";
        case TransactionStatus::RISK_APPROVED:   return "RISK_APPROVED";
        case TransactionStatus::RISK_CHALLENGED: return "RISK_CHALLENGED";
        case TransactionStatus::RISK_REJECTED:   return "RISK_REJECTED";
        case TransactionStatus::AUTHORIZED:      return "AUTHORIZED";
        case TransactionStatus::CAPTURED:        return "CAPTURED";
        case TransactionStatus::SETTLED:         return "SETTLED";
        case TransactionStatus::DECLINED:        return "DECLINED";
        case TransactionStatus::REFUNDED:        return "REFUNDED";
        case TransactionStatus::FAILED:          return "FAILED";
        default:                                 return "UNKNOWN";
    }
}

/**
 * @brief Risk Action recommendations
 */
enum class RiskAction {
    APPROVE,
    CHALLENGE_3DS,
    MANUAL_REVIEW,
    DECLINE_FRAUD
};

inline std::string riskActionToString(RiskAction action) {
    switch (action) {
        case RiskAction::APPROVE:        return "APPROVE";
        case RiskAction::CHALLENGE_3DS:  return "CHALLENGE_3DS";
        case RiskAction::MANUAL_REVIEW:  return "MANUAL_REVIEW";
        case RiskAction::DECLINE_FRAUD:  return "DECLINE_FRAUD";
        default:                         return "UNKNOWN";
    }
}

/**
 * @brief Risk Evaluation result
 */
struct RiskEvaluation {
    double score{0.0};                     // 0.0 (safest) to 100.0 (highest risk)
    RiskAction action{RiskAction::APPROVE};
    std::vector<std::string> ruleTriggers;
    std::string evaluationNotes;
    uint64_t evaluationMicros{0};
};

/**
 * @brief Geo-location coordinates (latitude, longitude)
 */
struct GeoLocation {
    double latitude{0.0};
    double longitude{0.0};
    std::string countryCode{"US"};
    std::string city{"Unknown"};
};

/**
 * @brief Contextual data accompanying a transaction request
 */
struct TransactionContext {
    std::string transactionId;
    std::string accountId;
    std::string merchantId;
    std::string terminalId;
    double amount{0.0};
    std::string currency{"USD"};
    InstrumentType instrumentType{InstrumentType::CREDIT_CARD};
    GeoLocation location;
    std::string ipAddress{"127.0.0.1"};
    std::string deviceFingerprint{"device-unknown"};
    std::chrono::system_clock::time_point timestamp{std::chrono::system_clock::now()};
    bool offlineQueued{false};
};

/**
 * @brief Transaction Execution Result
 */
struct TransactionResult {
    std::string transactionId;
    TransactionStatus status{TransactionStatus::INITIATED};
    std::string authorizationCode;
    RiskEvaluation riskResult;
    double authorizedAmount{0.0};
    std::string currency{"USD"};
    std::string maskedInstrument;
    std::string responseCode{"00"}; // ISO 8583 response: "00" = Approved
    std::string responseMessage{"Approved"};
    uint64_t latencyMicros{0};
    std::chrono::system_clock::time_point completedAt{std::chrono::system_clock::now()};
};

/**
 * @brief Authorization Result for payment instruments
 */
struct AuthorizationResult {
    bool success{false};
    std::string authCode;
    std::string responseCode{"00"};
    std::string message{"Success"};
    double fee{0.0};
};

/**
 * @brief Capture and Refund results
 */
struct CaptureResult {
    bool success{false};
    std::string captureId;
    std::string responseCode{"00"};
    std::string message{"Captured"};
};

struct RefundResult {
    bool success{false};
    std::string refundId;
    double amountRefunded{0.0};
    std::string responseCode{"00"};
    std::string message{"Refunded"};
};

} // namespace omnipay
