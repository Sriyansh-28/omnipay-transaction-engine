//
//  OmniPayCheckoutViewController.m
//  OmniPay iOS Mobile SDK - Responsive Checkout View Controller
//

#import "OmniPayCheckoutViewController.h"

#if TARGET_OS_IOS || TARGET_OS_IPHONE

@interface OmniPayCheckoutViewController () <UITextFieldDelegate>

@property (nonatomic, strong) UIScrollView *scrollView;
@property (nonatomic, strong) UIView *contentView;
@property (nonatomic, strong) UILabel *titleLabel;
@property (nonatomic, strong) UILabel *securityBadgeLabel;
@property (nonatomic, strong) UIView *offlineBannerView;
@property (nonatomic, strong) UITextField *cardNumberField;
@property (nonatomic, strong) UITextField *cardholderNameField;
@property (nonatomic, strong) UITextField *expiryField;
@property (nonatomic, strong) UITextField *cvvField;
@property (nonatomic, strong) UIButton *payButton;
@property (nonatomic, strong) PKPaymentButton *applePayButton;
@property (nonatomic, strong) UIImageView *brandImageView;

@end

@implementation OmniPayCheckoutViewController

- (instancetype)initWithPaymentRequest:(OmniPayPaymentRequest *)request {
    self = [super initWithNibName:nil bundle:nil];
    if (self) {
        _paymentRequest = request;
    }
    return self;
}

- (void)viewDidLoad {
    [super viewDidLoad];
    self.view.backgroundColor = [UIColor systemGroupedBackgroundColor];
    self.title = @"OmniPay Checkout";

    [self setupViewHierarchy];
    [self setupAccessibility];
    [self setupActions];
}

- (void)setupViewHierarchy {
    self.scrollView = [[UIScrollView alloc] initWithFrame:self.view.bounds];
    self.scrollView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    [self.view addSubview:self.scrollView];

    self.contentView = [[UIView alloc] initWithFrame:CGRectMake(16, 20, self.view.bounds.size.width - 32, 600)];
    [self.scrollView addSubview:self.contentView];

    // Security badge
    self.securityBadgeLabel = [[UILabel alloc] initWithFrame:CGRectMake(0, 0, self.contentView.bounds.size.width, 24)];
    self.securityBadgeLabel.text = @"🔒 256-bit Encrypted Checkout";
    self.securityBadgeLabel.textColor = [UIColor secondaryLabelColor];
    self.securityBadgeLabel.font = [UIFont systemFontOfSize:13 weight:UIFontWeightMedium];
    [self.contentView addSubview:self.securityBadgeLabel];

    // Card Number
    self.cardNumberField = [[UITextField alloc] initWithFrame:CGRectMake(0, 40, self.contentView.bounds.size.width, 48)];
    self.cardNumberField.borderStyle = UITextBorderStyleRoundedRect;
    self.cardNumberField.placeholder = @"4111 2222 3333 4444";
    self.cardNumberField.keyboardType = UIKeyboardTypeNumberPad;
    self.cardNumberField.delegate = self;
    [self.contentView addSubview:self.cardNumberField];

    // Cardholder Name
    self.cardholderNameField = [[UITextField alloc] initWithFrame:CGRectMake(0, 100, self.contentView.bounds.size.width, 48)];
    self.cardholderNameField.borderStyle = UITextBorderStyleRoundedRect;
    self.cardholderNameField.placeholder = @"Cardholder Name";
    self.cardholderNameField.autocapitalizationType = UITextAutocapitalizationTypeWords;
    [self.contentView addSubview:self.cardholderNameField];

    // Side-by-side Expiry and CVV
    CGFloat halfWidth = (self.contentView.bounds.size.width - 16) / 2.0;
    self.expiryField = [[UITextField alloc] initWithFrame:CGRectMake(0, 160, halfWidth, 48)];
    self.expiryField.borderStyle = UITextBorderStyleRoundedRect;
    self.expiryField.placeholder = @"MM/YY";
    self.expiryField.keyboardType = UIKeyboardTypeNumberPad;
    [self.contentView addSubview:self.expiryField];

    self.cvvField = [[UITextField alloc] initWithFrame:CGRectMake(halfWidth + 16, 160, halfWidth, 48)];
    self.cvvField.borderStyle = UITextBorderStyleRoundedRect;
    self.cvvField.placeholder = @"CVV";
    self.cvvField.keyboardType = UIKeyboardTypeNumberPad;
    self.cvvField.secureTextEntry = YES;
    [self.contentView addSubview:self.cvvField];

    // Pay Button (WCAG 2.1 AA 48dp+ touch target)
    self.payButton = [UIButton buttonWithType:UIButtonTypeSystem];
    self.payButton.frame = CGRectMake(0, 240, self.contentView.bounds.size.width, 54);
    self.payButton.backgroundColor = [UIColor systemBlueColor];
    [self.payButton setTitleColor:[UIColor whiteColor] forState:UIControlStateNormal];
    [self.payButton setTitle:[NSString stringWithFormat:@"Pay $%.2f", self.paymentRequest.amount > 0 ? self.paymentRequest.amount : 49.99] forState:UIControlStateNormal];
    self.payButton.titleLabel.font = [UIFont systemFontOfSize:17 weight:UIFontWeightBold];
    self.payButton.layer.cornerRadius = 10;
    [self.contentView addSubview:self.payButton];

    // Apple Pay Button
    if ([PKPaymentAuthorizationViewController canMakePayments]) {
        self.applePayButton = [PKPaymentButton buttonWithType:PKPaymentButtonTypeBuy style:PKPaymentButtonStyleBlack];
        self.applePayButton.frame = CGRectMake(0, 310, self.contentView.bounds.size.width, 54);
        self.applePayButton.layer.cornerRadius = 10;
        [self.contentView addSubview:self.applePayButton];
    }
}

- (void)setupAccessibility {
    self.securityBadgeLabel.isAccessibilityElement = YES;
    self.securityBadgeLabel.accessibilityLabel = @"Secured with 256 bit encryption";

    self.cardNumberField.isAccessibilityElement = YES;
    self.cardNumberField.accessibilityLabel = @"Card Number";
    self.cardNumberField.accessibilityHint = @"Enter your 16 digit credit or debit card number";

    self.cardholderNameField.isAccessibilityElement = YES;
    self.cardholderNameField.accessibilityLabel = @"Cardholder Full Name";

    self.expiryField.isAccessibilityElement = YES;
    self.expiryField.accessibilityLabel = @"Expiration Date in Month and Year format";

    self.cvvField.isAccessibilityElement = YES;
    self.cvvField.accessibilityLabel = @"Card Security Code CVV";
    self.cvvField.accessibilityHint = @"3 or 4 digit code on the back of your card";

    self.payButton.isAccessibilityElement = YES;
    self.payButton.accessibilityLabel = [NSString stringWithFormat:@"Submit payment of %.2f dollars", self.paymentRequest.amount > 0 ? self.paymentRequest.amount : 49.99];
    self.payButton.accessibilityTraits = UIAccessibilityTraitButton;
}

- (void)setupActions {
    [self.payButton addTarget:self action:@selector(handlePayButtonTapped) forControlEvents:UIControlEventTouchUpInside];
    if (self.applePayButton) {
        [self.applePayButton addTarget:self action:@selector(handleApplePayTapped) forControlEvents:UIControlEventTouchUpInside];
    }
}

- (void)handlePayButtonTapped {
    NSString *pan = self.cardNumberField.text ?: @"";
    NSString *name = self.cardholderNameField.text ?: @"";
    NSString *exp = self.expiryField.text ?: @"";
    NSString *cvv = self.cvvField.text ?: @"";

    NSInteger month = 12, year = 2028;
    if ([exp containsString:@"/"]) {
        NSArray *p = [exp componentsSeparatedByString:@"/"];
        month = [p[0] integerValue];
        year = 2000 + [p[1] integerValue];
    }

    OmniPayCard *card = [[OmniPayCard alloc] initWithCardNumber:pan
                                                 cardholderName:name
                                                    expiryMonth:month
                                                     expiryYear:year
                                                            cvv:cvv
                                                     billingZip:@"94105"];

    if (![card isValid]) {
        UIAlertController *alert = [UIAlertController alertControllerWithTitle:@"Validation Error"
                                                                       message:@"Please check your card details and try again."
                                                                preferredStyle:UIAlertControllerStyleAlert];
        [alert addAction:[UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault handler:nil]];
        [self presentViewController:alert animated:YES completion:nil];
        return;
    }

    self.payButton.enabled = NO;
    [self.payButton setTitle:@"Authorizing..." forState:UIControlStateNormal];

    OmniPayPaymentRequest *req = [OmniPayPaymentRequest requestWithAmount:49.99
                                                                 currency:@"USD"
                                                               merchantId:@"MERCH-IOS-01"
                                                               customerId:@"CUST-IOS-99"
                                                                     card:card];

    [[OmniPayManager sharedManager] authorizePayment:req completion:^(OmniPayPaymentResponse * _Nullable response, NSError * _Nullable error) {
        self.payButton.enabled = YES;
        [self.payButton setTitle:@"Pay $49.99" forState:UIControlStateNormal];

        if (response && response.success) {
            if (self.onPaymentSuccess) {
                self.onPaymentSuccess(response);
            }
            [self dismissViewControllerAnimated:YES completion:nil];
        } else {
            if (self.onPaymentFailure) {
                self.onPaymentFailure(error);
            }
        }
    }];
}

- (void)handleApplePayTapped {
    PKPaymentRequest *pkReq = [[PKPaymentRequest alloc] init];
    pkReq.merchantIdentifier = @"merchant.com.omnipay.demo";
    pkReq.supportedNetworks = @[PKPaymentNetworkVisa, PKPaymentNetworkMasterCard, PKPaymentNetworkAmex];
    pkReq.merchantCapabilities = PKMerchantCapability3DS;
    pkReq.countryCode = @"US";
    pkReq.currencyCode = @"USD";
    pkReq.paymentSummaryItems = @[[PKPaymentSummaryItem summaryItemWithLabel:@"OmniPay Goods" amount:[NSDecimalNumber decimalNumberWithString:@"49.99"]]];

    PKPaymentAuthorizationViewController *authVC = [[PKPaymentAuthorizationViewController alloc] initWithPaymentRequest:pkReq];
    authVC.delegate = self;
    [self presentViewController:authVC animated:YES completion:nil];
}

#pragma mark - PKPaymentAuthorizationViewControllerDelegate

- (void)paymentAuthorizationViewController:(PKPaymentAuthorizationViewController *)controller
                       didAuthorizePayment:(PKPayment *)payment
                                completion:(void (^)(PKPaymentAuthorizationStatus status))completion {
    completion(PKPaymentAuthorizationStatusSuccess);
}

- (void)paymentAuthorizationViewControllerDidFinish:(PKPaymentAuthorizationViewController *)controller {
    [self dismissViewControllerAnimated:YES completion:nil];
}

@end

#endif
