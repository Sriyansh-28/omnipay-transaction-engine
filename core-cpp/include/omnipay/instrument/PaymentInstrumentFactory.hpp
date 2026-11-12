#pragma once

#include "omnipay/instrument/IPaymentInstrument.hpp"
#include "omnipay/instrument/CreditCardPayment.hpp"
#include "omnipay/instrument/DebitCardPayment.hpp"
#include "omnipay/instrument/NFCTokenPayment.hpp"
#include "omnipay/instrument/QRWalletPayment.hpp"
#include <memory>
#include <map>
#include <string>

namespace omnipay {

/**
 * @brief Factory Design Pattern for polymorphic payment instrument instantiation.
 * Strictly adheres to GoF Creational Design Patterns, RAII, and encapsulation.
 */
class PaymentInstrumentFactory {
public:
    PaymentInstrumentFactory() = delete; // Static factory utility

    static std::unique_ptr<IPaymentInstrument> createCreditCard(
        const std::string& cardNumber,
        const std::string& cardholderName,
        int expiryMonth,
        int expiryYear,
        const std::string& cvv,
        const std::string& billingZip = "",
        bool is3dsEnrolled = true);

    static std::unique_ptr<IPaymentInstrument> createDebitCard(
        const std::string& cardNumber,
        const std::string& cardholderName,
        int expiryMonth,
        int expiryYear,
        const std::string& pinBlockToken,
        DebitAccountType accountType = DebitAccountType::CHECKING,
        double availableBalance = 5000.0,
        double dailyLimit = 2500.0);

    static std::unique_ptr<IPaymentInstrument> createNFCToken(
        const std::string& devicePAN,
        const std::string& tokenRequesterId,
        const std::string& dynamicCryptogram,
        uint32_t transactionCounter = 1,
        NFCPayProvider provider = NFCPayProvider::APPLE_PAY,
        bool biometricAuthenticated = true,
        int tokenAssuranceLevel = 60);

    static std::unique_ptr<IPaymentInstrument> createQRWallet(
        const std::string& qrPayload,
        const std::string& walletUserId,
        QRWalletProvider provider = QRWalletProvider::EMVCO_GENERIC,
        bool isDynamic = true,
        uint64_t ttlSeconds = 300,
        const std::string& signature = "SIG_VALID");

    /**
     * @brief Universal dictionary-based factory creator (useful for JSON / Mobile SDK RPC bridges)
     */
    static std::unique_ptr<IPaymentInstrument> createFromParameters(
        InstrumentType type,
        const std::map<std::string, std::string>& params);
};

} // namespace omnipay
