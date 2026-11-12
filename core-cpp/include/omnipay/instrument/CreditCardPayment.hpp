#pragma once

#include "omnipay/instrument/IPaymentInstrument.hpp"
#include <string>

namespace omnipay {

/**
 * @brief Credit Card Payment Instrument
 * Encapsulates cardholder PAN, CVV, expiry, brand detection, and 3DS authorization.
 */
class CreditCardPayment : public IPaymentInstrument {
public:
    CreditCardPayment(std::string cardNumber,
                      std::string cardholderName,
                      int expiryMonth,
                      int expiryYear,
                      std::string cvv,
                      std::string billingZip = "",
                      bool is3dsEnrolled = true);

    InstrumentType getType() const override { return InstrumentType::CREDIT_CARD; }
    std::string getMaskedIdentifier() const override;
    std::string getBrand() const override;
    bool validate() const override;
    AuthorizationResult authorize(double amount, 
                                  const std::string& currency, 
                                  const RiskEvaluation& risk) override;
    CaptureResult capture(const std::string& authCode, double amount) override;
    RefundResult refund(const std::string& transactionId, double amount, const std::string& reason) override;

    std::unique_ptr<IPaymentInstrument> clone() const override;

    // Specific accessors (Encapsulation)
    const std::string& getCardholderName() const { return cardholderName_; }
    int getExpiryMonth() const { return expiryMonth_; }
    int getExpiryYear() const { return expiryYear_; }
    bool is3dsEnrolled() const { return is3dsEnrolled_; }

private:
    std::string cardNumber_;
    std::string cardholderName_;
    int expiryMonth_{0};
    int expiryYear_{0};
    std::string cvv_;
    std::string billingZip_;
    bool is3dsEnrolled_{true};

    std::string detectBrand() const;
};

} // namespace omnipay
