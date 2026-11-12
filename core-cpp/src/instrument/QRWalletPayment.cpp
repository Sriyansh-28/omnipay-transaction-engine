#include "omnipay/instrument/QRWalletPayment.hpp"
#include <iomanip>
#include <sstream>
#include <chrono>

namespace omnipay {

QRWalletPayment::QRWalletPayment(std::string qrPayload,
                                 std::string walletUserId,
                                 QRWalletProvider provider,
                                 bool isDynamic,
                                 uint64_t ttlSeconds,
                                 std::string signature)
    : qrPayload_(std::move(qrPayload)),
      walletUserId_(std::move(walletUserId)),
      provider_(provider),
      isDynamic_(isDynamic),
      ttlSeconds_(ttlSeconds),
      createdAt_(std::chrono::system_clock::now()),
      signature_(std::move(signature)) {}

std::string QRWalletPayment::getBrand() const {
    return qrWalletProviderToString(provider_);
}

std::string QRWalletPayment::getMaskedIdentifier() const {
    std::string maskedId = walletUserId_.length() > 4 ?
        walletUserId_.substr(0, 2) + "****" + walletUserId_.substr(walletUserId_.length() - 2) : "USER_QR";
    return getBrand() + ":" + maskedId;
}

bool QRWalletPayment::isExpired() const {
    auto now = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - createdAt_).count();
    return elapsed > static_cast<int64_t>(ttlSeconds_);
}

bool QRWalletPayment::validate() const {
    if (qrPayload_.empty() || walletUserId_.empty()) {
        return false;
    }
    if (signature_.empty() || signature_ == "SIG_INVALID") {
        return false;
    }
    if (isDynamic_ && isExpired()) {
        return false;
    }
    return true;
}

AuthorizationResult QRWalletPayment::authorize(double amount, 
                                               const std::string& currency, 
                                               const RiskEvaluation& risk) {
    AuthorizationResult res;
    (void)currency;

    if (!validate()) {
        res.success = false;
        res.responseCode = "80"; // Invalid transaction / expired QR
        res.message = isExpired() ? "QR Code has expired" : "Malformed QR payload or invalid signature";
        return res;
    }

    if (amount <= 0.0) {
        res.success = false;
        res.responseCode = "13";
        res.message = "Transaction amount must be positive";
        return res;
    }

    if (risk.action == RiskAction::DECLINE_FRAUD) {
        res.success = false;
        res.responseCode = "05";
        res.message = "Declined by QR risk gateway rules";
        return res;
    }

    // QR Wallet fee (standard low-cost wallet rail: 0.8% + $0.05)
    res.fee = (amount * 0.008) + 0.05;
    res.success = true;
    res.responseCode = "00";

    auto nowNs = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    std::stringstream ss;
    ss << "AUTH-QR-" << std::hex << (nowNs & 0xFFFFFFFF);
    res.authCode = ss.str();
    res.message = "Approved (" + getBrand() + " EMVCo QR Dynamic Payload Cleared)";

    return res;
}

CaptureResult QRWalletPayment::capture(const std::string& authCode, double amount) {
    CaptureResult res;
    (void)amount;
    res.success = true;
    res.responseCode = "00";
    res.captureId = "CAP-QR-" + authCode;
    res.message = "Wallet capture executed";
    return res;
}

RefundResult QRWalletPayment::refund(const std::string& transactionId, double amount, const std::string& reason) {
    RefundResult res;
    (void)reason;
    res.success = true;
    res.amountRefunded = amount;
    res.refundId = "REF-QR-" + transactionId;
    res.responseCode = "00";
    res.message = "Instant refund credited to wallet account";
    return res;
}

std::unique_ptr<IPaymentInstrument> QRWalletPayment::clone() const {
    return std::make_unique<QRWalletPayment>(*this);
}

} // namespace omnipay
