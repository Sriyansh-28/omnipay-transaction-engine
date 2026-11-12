#pragma once

#include "omnipay/instrument/IPaymentInstrument.hpp"
#include <string>

namespace omnipay {

enum class NFCPayProvider {
    APPLE_PAY,
    GOOGLE_PAY,
    SAMSUNG_PAY,
    CONTACTLESS_EMV
};

inline std::string nfcPayProviderToString(NFCPayProvider provider) {
    switch (provider) {
        case NFCPayProvider::APPLE_PAY:       return "APPLE_PAY";
        case NFCPayProvider::GOOGLE_PAY:      return "GOOGLE_PAY";
        case NFCPayProvider::SAMSUNG_PAY:     return "SAMSUNG_PAY";
        case NFCPayProvider::CONTACTLESS_EMV: return "CONTACTLESS_EMV";
        default:                              return "NFC_GENERIC";
    }
}

/**
 * @brief NFC Tokenized Payment Instrument
 * Encapsulates EMV Contactless specifications, Apple Pay / Google Pay DPANs,
 * dynamic ARQC cryptograms, and biometric authorization tokens.
 */
class NFCTokenPayment : public IPaymentInstrument {
public:
    NFCTokenPayment(std::string devicePAN,
                    std::string tokenRequesterId,
                    std::string dynamicCryptogram,
                    uint32_t transactionCounter,
                    NFCPayProvider provider = NFCPayProvider::APPLE_PAY,
                    bool biometricAuthenticated = true,
                    int tokenAssuranceLevel = 60);

    InstrumentType getType() const override { return InstrumentType::NFC_TOKEN; }
    std::string getMaskedIdentifier() const override;
    std::string getBrand() const override;
    bool validate() const override;
    AuthorizationResult authorize(double amount, 
                                  const std::string& currency, 
                                  const RiskEvaluation& risk) override;
    CaptureResult capture(const std::string& authCode, double amount) override;
    RefundResult refund(const std::string& transactionId, double amount, const std::string& reason) override;

    std::unique_ptr<IPaymentInstrument> clone() const override;

    NFCPayProvider getProvider() const { return provider_; }
    const std::string& getCryptogram() const { return dynamicCryptogram_; }
    uint32_t getTransactionCounter() const { return transactionCounter_; }
    bool isBiometricAuthenticated() const { return biometricAuthenticated_; }
    int getTokenAssuranceLevel() const { return tokenAssuranceLevel_; }

private:
    std::string devicePAN_;
    std::string tokenRequesterId_;
    std::string dynamicCryptogram_;
    uint32_t transactionCounter_{1};
    NFCPayProvider provider_{NFCPayProvider::APPLE_PAY};
    bool biometricAuthenticated_{true};
    int tokenAssuranceLevel_{60};
};

} // namespace omnipay
