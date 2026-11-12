#include "omnipay/instrument/IPaymentInstrument.hpp"
#include <algorithm>
#include <cctype>

namespace omnipay {

bool IPaymentInstrument::luhnCheck(const std::string& cardNumber) {
    std::string sanitized;
    for (char c : cardNumber) {
        if (std::isdigit(static_cast<unsigned char>(c))) {
            sanitized.push_back(c);
        }
    }

    if (sanitized.length() < 12 || sanitized.length() > 19) {
        return false;
    }

    int sum = 0;
    bool alternate = false;
    for (int i = static_cast<int>(sanitized.length()) - 1; i >= 0; --i) {
        int n = sanitized[i] - '0';
        if (alternate) {
            n *= 2;
            if (n > 9) {
                n = (n % 10) + 1;
            }
        }
        sum += n;
        alternate = !alternate;
    }

    return (sum % 10 == 0);
}

std::string IPaymentInstrument::maskPan(const std::string& pan) {
    std::string sanitized;
    for (char c : pan) {
        if (std::isdigit(static_cast<unsigned char>(c))) {
            sanitized.push_back(c);
        }
    }

    if (sanitized.length() < 10) {
        return "XXXX-****";
    }

    std::string firstSix = sanitized.substr(0, 6);
    std::string lastFour = sanitized.substr(sanitized.length() - 4);
    std::string maskedMiddle(sanitized.length() - 10, 'X');

    return firstSix + maskedMiddle + lastFour;
}

} // namespace omnipay
