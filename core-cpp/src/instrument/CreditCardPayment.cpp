#include "omnipay/instrument/CreditCardPayment.hpp"
#include <iomanip>
#include <sstream>
#include <chrono>

namespace omnipay {

CreditCardPayment::CreditCardPayment(std::string cardNumber,
                                     std::string cardholderName,
                                     int expiryMonth,
                                     int expiryYear,
                                     std::string cvv,
                                     std::string billingZip,
                                     bool is3dsEnrolled)
    : cardNumber_(std::move(cardNumber)),
      cardholderName_(std::move(cardholderName)),
      expiryMonth_(expiryMonth),
      expiryYear_(expiryYear),
      cvv_(std::move(cvv)),
      billingZip_(std::move(billingZip)),
      is3dsEnrolled_(is3dsEnrolled) {}

std::string CreditCardPayment::detectBrand() const {
    if (cardNumber_.empty()) return "UNKNOWN";
    if (cardNumber_[0] == '4') return "VISA";
    if (cardNumber_.rfind("34", 0) == 0 || cardNumber_.rfind("37", 0) == 0) return "AMEX";
    if (cardNumber_.rfind("51", 0) == 0 || cardNumber_.rfind("52", 0) == 0 ||
        cardNumber_.rfind("53", 0) == 0 || cardNumber_.rfind("54", 0) == 0 ||
        cardNumber_.rfind("55", 0) == 0) return "MASTERCARD";
    if (cardNumber_.rfind("6011", 0) == 0 || cardNumber_.rfind("65", 0) == 0) return "DISCOVER";
    return "CREDIT_GENERIC";
}

std::string CreditCardPayment::getBrand() const {
    return detectBrand();
}

std::string CreditCardPayment::getMaskedIdentifier() const {
    return maskPan(cardNumber_);
}

bool CreditCardPayment::validate() const {
    if (!luhnCheck(cardNumber_)) {
        return false;
    }
    if (expiryMonth_ < 1 || expiryMonth_ > 12) {
        return false;
    }
    // Check expiry year (e.g. >= 2024)
    if (expiryYear_ < 2024 || expiryYear_ > 2040) {
        return false;
    }
    if (cvv_.length() < 3 || cvv_.length() > 4) {
        return false;
    }
    return true;
}

AuthorizationResult CreditCardPayment::authorize(double amount, 
                                                 const std::string& currency, 
                                                 const RiskEvaluation& risk) {
    AuthorizationResult res;
    (void)currency;

    if (!validate()) {
        res.success = false;
        res.responseCode = "14"; // Invalid card number
        res.message = "Card validation failed (Luhn/Expiry/CVV mismatch)";
        return res;
    }

    if (amount <= 0.0) {
        res.success = false;
        res.responseCode = "13"; // Invalid amount
        res.message = "Transaction amount must be positive";
        return res;
    }

    if (risk.action == RiskAction::DECLINE_FRAUD) {
        res.success = false;
        res.responseCode = "05"; // Do not honor / fraud decline
        res.message = "Declined by automated risk engine: " + risk.evaluationNotes;
        return res;
    }

    if (risk.action == RiskAction::CHALLENGE_3DS) {
        if (!is3dsEnrolled_) {
            res.success = false;
            res.responseCode = "65"; // Exceeds withdrawal frequency / 3DS required
            res.message = "Step-up 3DS authentication required but card is not enrolled";
            return res;
        }
        // Simulated successful 3DS step-up challenge
    }

    // Interchange fee calculation (e.g., 1.5% + $0.10)
    res.fee = (amount * 0.015) + 0.10;
    res.success = true;
    res.responseCode = "00";

    // Generate unique auth code
    auto nowNs = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    std::stringstream ss;
    ss << "AUTH-CC-" << std::hex << (nowNs & 0xFFFFFFFF);
    res.authCode = ss.str();
    res.message = "Approved (" + detectBrand() + " Interchange Settled)";

    return res;
}

CaptureResult CreditCardPayment::capture(const std::string& authCode, double amount) {
    CaptureResult res;
    (void)amount;
    if (authCode.empty()) {
        res.success = false;
        res.responseCode = "12";
        res.message = "Invalid auth code for capture";
        return res;
    }

    res.success = true;
    res.responseCode = "00";
    res.captureId = "CAP-" + authCode.substr(authCode.length() > 8 ? authCode.length() - 8 : 0);
    res.message = "Funds captured successfully";
    return res;
}

RefundResult CreditCardPayment::refund(const std::string& transactionId, double amount, const std::string& reason) {
    RefundResult res;
    (void)reason;
    res.success = true;
    res.amountRefunded = amount;
    res.refundId = "REF-" + transactionId;
    res.responseCode = "00";
    res.message = "Credit reversal authorized back to cardholder account";
    return res;
}

std::unique_ptr<IPaymentInstrument> CreditCardPayment::clone() const {
    return std::make_unique<CreditCardPayment>(*this);
}

} // namespace omnipay
