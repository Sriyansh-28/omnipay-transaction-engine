//
//  OmniPaySDK.h
//  OmniPay iOS Mobile SDK
//
//  Production-grade Objective-C payment SDK wrapper interfacing with
//  high-performance C++17 OmniPay transaction engine.
//

#import <Foundation/Foundation.h>
#if TARGET_OS_IOS || TARGET_OS_IPHONE
#import <UIKit/UIKit.h>
#else
// macOS / cross-platform compilation stubs
@interface UIViewController : NSObject
@end
#endif

NS_ASSUME_NONNULL_BEGIN

/**
 * Supported payment card brands
 */
typedef NS_ENUM(NSInteger, OmniPayCardBrand) {
    OmniPayCardBrandUnknown,
    OmniPayCardBrandVisa,
    OmniPayCardBrandMastercard,
    OmniPayCardBrandAmex,
    OmniPayCardBrandDiscover
};

/**
 * Card model with Luhn Mod-10 client-side validation and formatting
 */
@interface OmniPayCard : NSObject

@property (nonatomic, copy, readonly) NSString *cardNumber;
@property (nonatomic, copy, readonly) NSString *cardholderName;
@property (nonatomic, assign, readonly) NSInteger expiryMonth;
@property (nonatomic, assign, readonly) NSInteger expiryYear;
@property (nonatomic, copy, readonly) NSString *cvv;
@property (nonatomic, copy, readonly, nullable) NSString *billingZip;

- (instancetype)initWithCardNumber:(NSString *)cardNumber
                    cardholderName:(NSString *)cardholderName
                       expiryMonth:(NSInteger)expiryMonth
                        expiryYear:(NSInteger)expiryYear
                               cvv:(NSString *)cvv
                        billingZip:(nullable NSString *)billingZip;

- (BOOL)isValidLuhn;
- (BOOL)isValidExpiry;
- (BOOL)isValidCvv;
- (BOOL)isValid;
- (NSString *)maskedCardNumber;
- (OmniPayCardBrand)detectedBrand;
- (NSString *)brandName;

@end

/**
 * Mobile transaction authorization request
 */
@interface OmniPayPaymentRequest : NSObject

@property (nonatomic, copy, readonly) NSString *transactionId;
@property (nonatomic, assign, readonly) double amount;
@property (nonatomic, copy, readonly) NSString *currency;
@property (nonatomic, copy, readonly) NSString *merchantId;
@property (nonatomic, copy, readonly) NSString *customerId;
@property (nonatomic, strong, readonly, nullable) OmniPayCard *card;
@property (nonatomic, copy, readonly, nullable) NSString *token;
@property (nonatomic, assign, readonly) BOOL offlineQueued;

+ (instancetype)requestWithAmount:(double)amount
                         currency:(NSString *)currency
                       merchantId:(NSString *)merchantId
                       customerId:(NSString *)customerId
                             card:(OmniPayCard *)card;

+ (instancetype)requestWithAmount:(double)amount
                         currency:(NSString *)currency
                       merchantId:(NSString *)merchantId
                       customerId:(NSString *)customerId
                            token:(NSString *)token;

@end

/**
 * Mobile transaction authorization response
 */
@interface OmniPayPaymentResponse : NSObject

@property (nonatomic, assign, readonly) BOOL success;
@property (nonatomic, copy, readonly) NSString *transactionId;
@property (nonatomic, copy, readonly) NSString *authCode;
@property (nonatomic, copy, readonly) NSString *responseCode;
@property (nonatomic, copy, readonly) NSString *message;
@property (nonatomic, copy, readonly) NSString *maskedIdentifier;
@property (nonatomic, assign, readonly) uint64_t latencyMicros;
@property (nonatomic, assign, readonly) BOOL offlineProcessed;

- (instancetype)initWithSuccess:(BOOL)success
                  transactionId:(NSString *)transactionId
                       authCode:(NSString *)authCode
                   responseCode:(NSString *)responseCode
                        message:(NSString *)message
               maskedIdentifier:(NSString *)maskedIdentifier
                  latencyMicros:(uint64_t)latencyMicros
               offlineProcessed:(BOOL)offlineProcessed;

@end

// Asynchronous callback blocks
typedef void (^OmniPayTokenCompletion)(NSString * _Nullable token, NSString * _Nullable maskedPan, NSError * _Nullable error);
typedef void (^OmniPayPaymentCompletion)(OmniPayPaymentResponse * _Nullable response, NSError * _Nullable error);
typedef void (^OmniPaySyncCompletion)(NSInteger syncedCount, NSInteger failedCount);

/**
 * OmniPay SDK Manager: Main Objective-C Singleton interface.
 * Implements tokenization, offline SQLite/file persistence, and C++ engine bridge.
 */
@interface OmniPayManager : NSObject

@property (nonatomic, copy, readonly, nullable) NSString *publishableKey;
@property (nonatomic, copy, readonly, nullable) NSString *merchantId;
@property (nonatomic, assign, readonly) BOOL isConfigured;
@property (nonatomic, assign, getter=isNetworkConnected) BOOL networkConnected;

+ (instancetype)sharedManager;

- (void)configureWithApiKey:(NSString *)apiKey merchantId:(NSString *)merchantId;

- (void)tokenizeCard:(OmniPayCard *)card
          completion:(OmniPayTokenCompletion)completion;

- (void)authorizePayment:(OmniPayPaymentRequest *)request
              completion:(OmniPayPaymentCompletion)completion;

- (void)queueOfflinePayment:(OmniPayPaymentRequest *)request;

- (void)syncOfflineQueueWithCompletion:(nullable OmniPaySyncCompletion)completion;

- (OmniPayPaymentResponse *)invokeCppCoreBridgeWithRequest:(OmniPayPaymentRequest *)request;

- (NSInteger)offlineQueueCount;
- (void)clearOfflineQueue;

@end

/**
 * Mobile Checkout UI View Controller Stub
 */
@interface OmniPayCheckoutViewController : UIViewController

@property (nonatomic, strong) OmniPayPaymentRequest *paymentRequest;
@property (nonatomic, copy, nullable) void (^onPaymentSuccess)(OmniPayPaymentResponse *response);
@property (nonatomic, copy, nullable) void (^onPaymentFailure)(NSError *error);

- (instancetype)initWithPaymentRequest:(OmniPayPaymentRequest *)request;

@end

NS_ASSUME_NONNULL_END
