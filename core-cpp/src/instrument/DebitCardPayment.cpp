#include "omnipay/instrument/DebitCardPayment.hpp"
#include <iomanip>
#include <sstream>
#include <chrono>

namespace omnipay {

DebitCardPayment::DebitCardPayment(std::string cardNumber,
                                   std::string cardholderName,
                                   int expiryMonth,
                                   int expiryYear,
                                   std::string pinBlockToken,
                                   DebitAccountType accountType,
                                   double availableBalance,
                                   double dailyLimit)
    : cardNumber_(std::move(cardNumber)),
      cardholderName_(std::move(cardholderName)),
      expiryMonth_(expiryMonth),
      expiryYear_(expiryYear),
      pinBlockToken_(std::move(pinBlockToken)),
      accountType_(accountType),
      availableBalance_(availableBalance),
      dailyLimit_(dailyLimit) {}

std::string DebitCardPayment::getBrand() const {
    if (cardNumber_.empty()) return "DEBIT";
    if (cardNumber_[0] == '4') return "VISA_DEBIT";
    if (cardNumber_.rfind("51", 0) == 0 || cardNumber_.rfind("52", 0) == 0 ||
        cardNumber_.rfind("53", 0) == 0 || cardNumber_.rfind("54", 0) == 0 ||
        cardNumber_.rfind("55", 0) == 0) return "MASTERCARD_DEBIT";
    return "DEBIT_NETWORK";
}

std::string DebitCardPayment::getMaskedIdentifier() const {
    return maskPan(cardNumber_) + " [" + debitAccountTypeToString(accountType_) + "]";
}

bool DebitCardPayment::validate() const {
    if (!luhnCheck(cardNumber_)) {
        return false;
    }
    if (expiryMonth_ < 1 || expiryMonth_ > 12) {
        return false;
    }
    if (expiryYear_ < 2024 || expiryYear_ > 2040) {
        return false;
    }
    if (pinBlockToken_.empty()) {
        return false;
    }
    return true;
}

AuthorizationResult DebitCardPayment::authorize(double amount, 
                                                const std::string& currency, 
                                                const RiskEvaluation& risk) {
    AuthorizationResult res;
    (void)currency;

    if (!validate()) {
        res.success = false;
        res.responseCode = "14";
        res.message = "Debit card validation failed (Invalid card or missing PIN block)";
        return res;
    }

    if (amount <= 0.0) {
        res.success = false;
        res.responseCode = "13";
        res.message = "Transaction amount must be positive";
        return res;
    }

    if (amount > dailyLimit_) {
        res.success = false;
        res.responseCode = "61"; // Exceeds withdrawal amount limit
        res.message = "Transaction amount exceeds daily debit limit";
        return res;
    }

    if (amount > availableBalance_) {
        res.success = false;
        res.responseCode = "51"; // Insufficient funds
        res.message = "Insufficient funds in " + debitAccountTypeToString(accountType_) + " account";
        return res;
    }

    if (risk.action == RiskAction::DECLINE_FRAUD) {
        res.success = false;
        res.responseCode = "05";
        res.message = "Declined by debit risk verification rules";
        return res;
    }

    // Debit fee: lower interchange than credit (Durbin Amendment compliant: 0.05% + $0.21)
    res.fee = (amount * 0.0005) + 0.21;
    res.success = true;
    res.responseCode = "00";

    availableBalance_ -= amount; // Deduct balance

    auto nowNs = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    std::stringstream ss;
    ss << "AUTH-DB-" << std::hex << (nowNs & 0xFFFFFFFF);
    res.authCode = ss.str();
    res.message = "Approved (" + getBrand() + " Realtime Debit Hold Placed)";

    return res;
}

CaptureResult DebitCardPayment::capture(const std::string& authCode, double amount) {
    CaptureResult res;
    (void)amount;
    res.success = true;
    res.responseCode = "00";
    res.captureId = "CAP-DB-" + authCode;
    res.message = "Debit batch settlement confirmed";
    return res;
}

RefundResult DebitCardPayment::refund(const std::string& transactionId, double amount, const std::string& reason) {
    RefundResult res;
    (void)reason;
    availableBalance_ += amount;
    res.success = true;
    res.amountRefunded = amount;
    res.refundId = "REF-DB-" + transactionId;
    res.responseCode = "00";
    res.message = "Direct debit reversal credited back to ledger";
    return res;
}

std::unique_ptr<IPaymentInstrument> DebitCardPayment::clone() const {
    return std::make_unique<DebitCardPayment>(*this);
}

} // namespace omnipay
