#pragma once

#include "omnipay/types.hpp"
#include <memory>
#include <string>

namespace omnipay {

/**
 * @brief Abstract Base Class for all Payment Instruments
 * Demonstrates Object-Oriented Design (Polymorphism, Encapsulation, RAII)
 */
class IPaymentInstrument {
public:
    virtual ~IPaymentInstrument() = default;

    // Polymorphic core interface
    virtual InstrumentType getType() const = 0;
    virtual std::string getMaskedIdentifier() const = 0;
    virtual std::string getBrand() const = 0;
    virtual bool validate() const = 0;
    virtual AuthorizationResult authorize(double amount, 
                                          const std::string& currency, 
                                          const RiskEvaluation& risk) = 0;
    virtual CaptureResult capture(const std::string& authCode, double amount) = 0;
    virtual RefundResult refund(const std::string& transactionId, double amount, const std::string& reason) = 0;

    // Polymorphic copy / factory support
    virtual std::unique_ptr<IPaymentInstrument> clone() const = 0;

    // Base utility helpers
    static bool luhnCheck(const std::string& cardNumber);
    static std::string maskPan(const std::string& pan);
};

} // namespace omnipay
