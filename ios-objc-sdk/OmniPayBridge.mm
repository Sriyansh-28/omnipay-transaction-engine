//
//  OmniPayBridge.mm
//  OmniPay iOS Mobile SDK - Objective-C++ Bridge to C++17 Core
//

#import "OmniPaySDK.h"
#include "omnipay/engine/TransactionEngine.hpp"
#include "omnipay/instrument/PaymentInstrumentFactory.hpp"

@interface OmniPayBridge : NSObject
+ (OmniPayPaymentResponse *)processNativeTransactionWithRequest:(OmniPayPaymentRequest *)request;
@end

@implementation OmniPayBridge

+ (OmniPayPaymentResponse *)processNativeTransactionWithRequest:(OmniPayPaymentRequest *)request {
    static std::unique_ptr<omnipay::TransactionEngine> s_iosEngine = nullptr;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        omnipay::TransactionEngine::EngineConfig cfg;
        cfg.workerThreads = 2;
        cfg.enableDefaultObservers = true;
        cfg.enableDefaultRiskPipeline = true;
        s_iosEngine = std::make_unique<omnipay::TransactionEngine>(cfg);
    });

    omnipay::TransactionContext ctx;
    ctx.transactionId = std::string([request.transactionId UTF8String]);
    ctx.accountId = std::string([request.customerId UTF8String]);
    ctx.amount = request.amount;
    ctx.currency = std::string([request.currency UTF8String]);
    ctx.location = {37.7749, -122.4194, "US", "San Francisco"};

    std::shared_ptr<omnipay::IPaymentInstrument> instrument = nullptr;

    if (request.card) {
        ctx.instrumentType = omnipay::InstrumentType::CREDIT_CARD;
        instrument = omnipay::PaymentInstrumentFactory::createCreditCard(
            std::string([request.card.cardNumber UTF8String]),
            std::string([request.card.cardholderName UTF8String]),
            static_cast<int>(request.card.expiryMonth),
            static_cast<int>(request.card.expiryYear),
            std::string([request.card.cvv UTF8String]),
            request.card.billingZip ? std::string([request.card.billingZip UTF8String]) : "",
            true
        );
    } else {
        ctx.instrumentType = omnipay::InstrumentType::NFC_TOKEN;
        instrument = omnipay::PaymentInstrumentFactory::createNFCToken(
            "4888888888888888", "40010075050", "DYNAMIC_EMV_ARQC",
            1, omnipay::NFCPayProvider::APPLE_PAY, true, 60
        );
    }

    omnipay::TransactionResult cResult = s_iosEngine->processTransaction(ctx, *instrument);

    BOOL success = (cResult.status == omnipay::TransactionStatus::AUTHORIZED);
    NSString *authCode = [NSString stringWithUTF8String:cResult.authorizationCode.c_str()];
    NSString *respCode = [NSString stringWithUTF8String:cResult.responseCode.c_str()];
    NSString *respMsg = [NSString stringWithUTF8String:cResult.responseMessage.c_str()];
    NSString *masked = [NSString stringWithUTF8String:cResult.maskedInstrument.c_str()];

    return [[OmniPayPaymentResponse alloc] initWithSuccess:success
                                             transactionId:request.transactionId
                                                  authCode:authCode
                                              responseCode:respCode
                                                   message:respMsg
                                          maskedIdentifier:masked
                                             latencyMicros:cResult.latencyMicros
                                          offlineProcessed:NO];
}

@end
