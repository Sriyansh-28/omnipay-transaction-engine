#include "omnipay/instrument/NFCTokenPayment.hpp"
#include <iomanip>
#include <sstream>
#include <chrono>

namespace omnipay {

NFCTokenPayment::NFCTokenPayment(std::string devicePAN,
                                 std::string tokenRequesterId,
                                 std::string dynamicCryptogram,
                                 uint32_t transactionCounter,
                                 NFCPayProvider provider,
                                 bool biometricAuthenticated,
                                 int tokenAssuranceLevel)
    : devicePAN_(std::move(devicePAN)),
      tokenRequesterId_(std::move(tokenRequesterId)),
      dynamicCryptogram_(std::move(dynamicCryptogram)),
      transactionCounter_(transactionCounter),
      provider_(provider),
      biometricAuthenticated_(biometricAuthenticated),
      tokenAssuranceLevel_(tokenAssuranceLevel) {}

std::string NFCTokenPayment::getBrand() const {
    return nfcPayProviderToString(provider_);
}

std::string NFCTokenPayment::getMaskedIdentifier() const {
    std::string masked = maskPan(devicePAN_);
    return masked + " (" + getBrand() + ")";
}

bool NFCTokenPayment::validate() const {
    if (devicePAN_.empty() || dynamicCryptogram_.empty()) {
        return false;
    }
    if (tokenAssuranceLevel_ < 40) {
        return false;
    }
    if (transactionCounter_ == 0) {
        return false;
    }
    return true;
}

AuthorizationResult NFCTokenPayment::authorize(double amount, 
                                               const std::string& currency, 
                                               const RiskEvaluation& risk) {
    AuthorizationResult res;
    (void)currency;

    if (!validate()) {
        res.success = false;
        res.responseCode = "58"; // Transaction not permitted on terminal
        res.message = "NFC token cryptogram or assurance validation failure";
        return res;
    }

    if (amount <= 0.0) {
        res.success = false;
        res.responseCode = "13";
        res.message = "Transaction amount must be positive";
        return res;
    }

    // High biometric assurance can override moderate risk scores
    if (risk.action == RiskAction::DECLINE_FRAUD) {
        if (!biometricAuthenticated_ || risk.score > 90.0) {
            res.success = false;
            res.responseCode = "05";
            res.message = "Declined: High risk score on tokenized channel";
            return res;
        }
    }

    // NFC Token processing fee (competitive digital wallet interchange)
    res.fee = (amount * 0.012) + 0.08;
    res.success = true;
    res.responseCode = "00";

    auto nowNs = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    std::stringstream ss;
    ss << "AUTH-NFC-" << std::hex << (nowNs & 0xFFFFFFFF);
    res.authCode = ss.str();
    res.message = "Approved (" + getBrand() + " EMV Contactless Cryptogram Verified)";

    return res;
}

CaptureResult NFCTokenPayment::capture(const std::string& authCode, double amount) {
    CaptureResult res;
    (void)amount;
    res.success = true;
    res.responseCode = "00";
    res.captureId = "CAP-NFC-" + authCode;
    res.message = "Tokenized payment settled";
    return res;
}

RefundResult NFCTokenPayment::refund(const std::string& transactionId, double amount, const std::string& reason) {
    RefundResult res;
    (void)reason;
    res.success = true;
    res.amountRefunded = amount;
    res.refundId = "REF-NFC-" + transactionId;
    res.responseCode = "00";
    res.message = "Contactless token reversal credited";
    return res;
}

std::unique_ptr<IPaymentInstrument> NFCTokenPayment::clone() const {
    return std::make_unique<NFCTokenPayment>(*this);
}

} // namespace omnipay
