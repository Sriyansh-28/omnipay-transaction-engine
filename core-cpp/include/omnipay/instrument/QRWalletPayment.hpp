#pragma once

#include "omnipay/instrument/IPaymentInstrument.hpp"
#include <string>

namespace omnipay {

enum class QRWalletProvider {
    ALIPAY,
    WECHAT_PAY,
    PAYTM,
    UPI_QR,
    MERCADOPAGO,
    EMVCO_GENERIC
};

inline std::string qrWalletProviderToString(QRWalletProvider provider) {
    switch (provider) {
        case QRWalletProvider::ALIPAY:        return "ALIPAY";
        case QRWalletProvider::WECHAT_PAY:    return "WECHAT_PAY";
        case QRWalletProvider::PAYTM:         return "PAYTM";
        case QRWalletProvider::UPI_QR:        return "UPI_QR";
        case QRWalletProvider::MERCADOPAGO:   return "MERCADOPAGO";
        case QRWalletProvider::EMVCO_GENERIC: return "EMVCO_GENERIC";
        default:                              return "QR_UNKNOWN";
    }
}

/**
 * @brief QR Wallet Payment Instrument
 * Compliant with EMVCo QR Code specifications for Consumer/Merchant presented modes.
 */
class QRWalletPayment : public IPaymentInstrument {
public:
    QRWalletPayment(std::string qrPayload,
                    std::string walletUserId,
                    QRWalletProvider provider = QRWalletProvider::EMVCO_GENERIC,
                    bool isDynamic = true,
                    uint64_t ttlSeconds = 300,
                    std::string signature = "SIG_VALID");

    InstrumentType getType() const override { return InstrumentType::QR_WALLET; }
    std::string getMaskedIdentifier() const override;
    std::string getBrand() const override;
    bool validate() const override;
    AuthorizationResult authorize(double amount, 
                                  const std::string& currency, 
                                  const RiskEvaluation& risk) override;
    CaptureResult capture(const std::string& authCode, double amount) override;
    RefundResult refund(const std::string& transactionId, double amount, const std::string& reason) override;

    std::unique_ptr<IPaymentInstrument> clone() const override;

    QRWalletProvider getProvider() const { return provider_; }
    const std::string& getWalletUserId() const { return walletUserId_; }
    bool isDynamic() const { return isDynamic_; }
    bool isExpired() const;

private:
    std::string qrPayload_;
    std::string walletUserId_;
    QRWalletProvider provider_{QRWalletProvider::EMVCO_GENERIC};
    bool isDynamic_{true};
    uint64_t ttlSeconds_{300};
    std::chrono::system_clock::time_point createdAt_{std::chrono::system_clock::now()};
    std::string signature_;
};

} // namespace omnipay
