package com.omnipay.sdk.model;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import java.util.UUID;

/**
 * Mobile transaction request carrying amount, currency, merchant info, and instrument.
 */
public final class PaymentRequest {
    private final String transactionId;
    private final double amount;
    private final String currency;
    private final String merchantId;
    private final String customerId;
    private final CardDetails cardDetails;
    private final String nfcToken;
    private final String qrPayload;

    public PaymentRequest(double amount,
                          @NonNull String currency,
                          @NonNull String merchantId,
                          @NonNull String customerId,
                          @Nullable CardDetails cardDetails,
                          @Nullable String nfcToken,
                          @Nullable String qrPayload) {
        this.transactionId = "TX-MOB-" + UUID.randomUUID().toString().substring(0, 8).toUpperCase();
        this.amount = amount;
        this.currency = currency;
        this.merchantId = merchantId;
        this.customerId = customerId;
        this.cardDetails = cardDetails;
        this.nfcToken = nfcToken;
        this.qrPayload = qrPayload;
    }

    public static PaymentRequest createCardPayment(double amount,
                                                   @NonNull String currency,
                                                   @NonNull String merchantId,
                                                   @NonNull String customerId,
                                                   @NonNull CardDetails card) {
        return new PaymentRequest(amount, currency, merchantId, customerId, card, null, null);
    }

    public static PaymentRequest createNfcPayment(double amount,
                                                  @NonNull String currency,
                                                  @NonNull String merchantId,
                                                  @NonNull String customerId,
                                                  @NonNull String nfcToken) {
        return new PaymentRequest(amount, currency, merchantId, customerId, null, nfcToken, null);
    }

    public static PaymentRequest createQrPayment(double amount,
                                                 @NonNull String currency,
                                                 @NonNull String merchantId,
                                                 @NonNull String customerId,
                                                 @NonNull String qrPayload) {
        return new PaymentRequest(amount, currency, merchantId, customerId, null, null, qrPayload);
    }

    @NonNull public String getTransactionId() { return transactionId; }
    public double getAmount() { return amount; }
    @NonNull public String getCurrency() { return currency; }
    @NonNull public String getMerchantId() { return merchantId; }
    @NonNull public String getCustomerId() { return customerId; }
    @Nullable public CardDetails getCardDetails() { return cardDetails; }
    @Nullable public String getNfcToken() { return nfcToken; }
    @Nullable public String getQrPayload() { return qrPayload; }
}
