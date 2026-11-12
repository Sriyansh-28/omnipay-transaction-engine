#include "omnipay/instrument/PaymentInstrumentFactory.hpp"
#include <stdexcept>

namespace omnipay {

std::unique_ptr<IPaymentInstrument> PaymentInstrumentFactory::createCreditCard(
    const std::string& cardNumber,
    const std::string& cardholderName,
    int expiryMonth,
    int expiryYear,
    const std::string& cvv,
    const std::string& billingZip,
    bool is3dsEnrolled) {
    return std::make_unique<CreditCardPayment>(
        cardNumber, cardholderName, expiryMonth, expiryYear, cvv, billingZip, is3dsEnrolled);
}

std::unique_ptr<IPaymentInstrument> PaymentInstrumentFactory::createDebitCard(
    const std::string& cardNumber,
    const std::string& cardholderName,
    int expiryMonth,
    int expiryYear,
    const std::string& pinBlockToken,
    DebitAccountType accountType,
    double availableBalance,
    double dailyLimit) {
    return std::make_unique<DebitCardPayment>(
        cardNumber, cardholderName, expiryMonth, expiryYear, pinBlockToken, accountType, availableBalance, dailyLimit);
}

std::unique_ptr<IPaymentInstrument> PaymentInstrumentFactory::createNFCToken(
    const std::string& devicePAN,
    const std::string& tokenRequesterId,
    const std::string& dynamicCryptogram,
    uint32_t transactionCounter,
    NFCPayProvider provider,
    bool biometricAuthenticated,
    int tokenAssuranceLevel) {
    return std::make_unique<NFCTokenPayment>(
        devicePAN, tokenRequesterId, dynamicCryptogram, transactionCounter, provider, biometricAuthenticated, tokenAssuranceLevel);
}

std::unique_ptr<IPaymentInstrument> PaymentInstrumentFactory::createQRWallet(
    const std::string& qrPayload,
    const std::string& walletUserId,
    QRWalletProvider provider,
    bool isDynamic,
    uint64_t ttlSeconds,
    const std::string& signature) {
    return std::make_unique<QRWalletPayment>(
        qrPayload, walletUserId, provider, isDynamic, ttlSeconds, signature);
}

std::unique_ptr<IPaymentInstrument> PaymentInstrumentFactory::createFromParameters(
    InstrumentType type,
    const std::map<std::string, std::string>& params) {
    
    auto getParam = [&params](const std::string& key, const std::string& defVal = "") -> std::string {
        auto it = params.find(key);
        return (it != params.end()) ? it->second : defVal;
    };

    switch (type) {
        case InstrumentType::CREDIT_CARD: {
            std::string pan = getParam("cardNumber");
            std::string name = getParam("cardholderName", "Cardholder");
            int expM = std::stoi(getParam("expiryMonth", "12"));
            int expY = std::stoi(getParam("expiryYear", "2028"));
            std::string cvv = getParam("cvv", "123");
            std::string zip = getParam("billingZip", "94105");
            bool is3ds = getParam("is3dsEnrolled", "true") == "true";
            return createCreditCard(pan, name, expM, expY, cvv, zip, is3ds);
        }
        case InstrumentType::DEBIT_CARD: {
            std::string pan = getParam("cardNumber");
            std::string name = getParam("cardholderName", "Cardholder");
            int expM = std::stoi(getParam("expiryMonth", "12"));
            int expY = std::stoi(getParam("expiryYear", "2028"));
            std::string pin = getParam("pinBlockToken", "PIN_BLOCK_DUMMY");
            DebitAccountType acc = (getParam("accountType") == "SAVINGS") ? 
                                    DebitAccountType::SAVINGS : DebitAccountType::CHECKING;
            double bal = std::stod(getParam("availableBalance", "5000.0"));
            double limit = std::stod(getParam("dailyLimit", "2500.0"));
            return createDebitCard(pan, name, expM, expY, pin, acc, bal, limit);
        }
        case InstrumentType::NFC_TOKEN: {
            std::string dpan = getParam("devicePAN");
            std::string reqId = getParam("tokenRequesterId", "40010075050");
            std::string crypto = getParam("dynamicCryptogram", "AQIDBAUGBwgJCgsMDQ4PEA==");
            uint32_t counter = static_cast<uint32_t>(std::stoul(getParam("transactionCounter", "1")));
            std::string provStr = getParam("provider", "APPLE_PAY");
            NFCPayProvider prov = NFCPayProvider::APPLE_PAY;
            if (provStr == "GOOGLE_PAY") prov = NFCPayProvider::GOOGLE_PAY;
            else if (provStr == "SAMSUNG_PAY") prov = NFCPayProvider::SAMSUNG_PAY;
            else if (provStr == "CONTACTLESS_EMV") prov = NFCPayProvider::CONTACTLESS_EMV;
            bool bio = getParam("biometricAuthenticated", "true") == "true";
            int tal = std::stoi(getParam("tokenAssuranceLevel", "60"));
            return createNFCToken(dpan, reqId, crypto, counter, prov, bio, tal);
        }
        case InstrumentType::QR_WALLET: {
            std::string payload = getParam("qrPayload", "000201010212...EMVCO");
            std::string userId = getParam("walletUserId", "USR_8899");
            std::string provStr = getParam("provider", "EMVCO_GENERIC");
            QRWalletProvider prov = QRWalletProvider::EMVCO_GENERIC;
            if (provStr == "ALIPAY") prov = QRWalletProvider::ALIPAY;
            else if (provStr == "WECHAT_PAY") prov = QRWalletProvider::WECHAT_PAY;
            else if (provStr == "PAYTM") prov = QRWalletProvider::PAYTM;
            else if (provStr == "UPI_QR") prov = QRWalletProvider::UPI_QR;
            bool dynamic = getParam("isDynamic", "true") == "true";
            uint64_t ttl = static_cast<uint64_t>(std::stoull(getParam("ttlSeconds", "300")));
            std::string sig = getParam("signature", "SIG_VALID");
            return createQRWallet(payload, userId, prov, dynamic, ttl, sig);
        }
        default:
            throw std::invalid_argument("Unsupported payment instrument type in factory");
    }
}

} // namespace omnipay
