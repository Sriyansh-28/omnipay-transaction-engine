#pragma once

#include "omnipay/instrument/IPaymentInstrument.hpp"
#include <string>

namespace omnipay {

enum class DebitAccountType {
    CHECKING,
    SAVINGS
};

inline std::string debitAccountTypeToString(DebitAccountType type) {
    return (type == DebitAccountType::CHECKING) ? "CHECKING" : "SAVINGS";
}

/**
 * @brief Debit Card Payment Instrument
 * Encapsulates PIN block verification, account types, and real-time ledger debiting.
 */
class DebitCardPayment : public IPaymentInstrument {
public:
    DebitCardPayment(std::string cardNumber,
                     std::string cardholderName,
                     int expiryMonth,
                     int expiryYear,
                     std::string pinBlockToken,
                     DebitAccountType accountType = DebitAccountType::CHECKING,
                     double availableBalance = 5000.0,
                     double dailyLimit = 2500.0);

    InstrumentType getType() const override { return InstrumentType::DEBIT_CARD; }
    std::string getMaskedIdentifier() const override;
    std::string getBrand() const override;
    bool validate() const override;
    AuthorizationResult authorize(double amount, 
                                  const std::string& currency, 
                                  const RiskEvaluation& risk) override;
    CaptureResult capture(const std::string& authCode, double amount) override;
    RefundResult refund(const std::string& transactionId, double amount, const std::string& reason) override;

    std::unique_ptr<IPaymentInstrument> clone() const override;

    DebitAccountType getAccountType() const { return accountType_; }
    double getAvailableBalance() const { return availableBalance_; }
    double getDailyLimit() const { return dailyLimit_; }

private:
    std::string cardNumber_;
    std::string cardholderName_;
    int expiryMonth_{0};
    int expiryYear_{0};
    std::string pinBlockToken_;
    DebitAccountType accountType_{DebitAccountType::CHECKING};
    double availableBalance_{5000.0};
    double dailyLimit_{2500.0};
};

} // namespace omnipay
