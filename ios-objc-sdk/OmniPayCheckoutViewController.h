//
//  OmniPayCheckoutViewController.h
//  OmniPay iOS Mobile SDK - Responsive Checkout View Controller
//

#import <Foundation/Foundation.h>
#import "OmniPaySDK.h"

#if TARGET_OS_IOS || TARGET_OS_IPHONE
#import <UIKit/UIKit.h>
#import <PassKit/PassKit.h>

@interface OmniPayCheckoutViewController : UIViewController <PKPaymentAuthorizationViewControllerDelegate>

@property (nonatomic, strong) OmniPayPaymentRequest *paymentRequest;
@property (nonatomic, copy, nullable) void (^onPaymentSuccess)(OmniPayPaymentResponse *response);
@property (nonatomic, copy, nullable) void (^onPaymentFailure)(NSError *error);

- (instancetype)initWithPaymentRequest:(OmniPayPaymentRequest *)request;

@end
#endif
