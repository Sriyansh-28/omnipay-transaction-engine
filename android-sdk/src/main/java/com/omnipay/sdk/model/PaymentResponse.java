package com.omnipay.sdk.model;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

/**
 * Mobile transaction authorization response model.
 */
public final class PaymentResponse {
    private final boolean success;
    private final String transactionId;
    private final String authCode;
    private final String responseCode;
    private final String message;
    private final String maskedInstrument;
    private final long latencyMicros;
    private final boolean offlineProcessed;

    public PaymentResponse(boolean success,
                           @NonNull String transactionId,
                           @Nullable String authCode,
                           @NonNull String responseCode,
                           @NonNull String message,
                           @NonNull String maskedInstrument,
                           long latencyMicros,
                           boolean offlineProcessed) {
        this.success = success;
        this.transactionId = transactionId;
        this.authCode = authCode != null ? authCode : "";
        this.responseCode = responseCode;
        this.message = message;
        this.maskedInstrument = maskedInstrument;
        this.latencyMicros = latencyMicros;
        this.offlineProcessed = offlineProcessed;
    }

    public boolean isSuccess() { return success; }
    @NonNull public String getTransactionId() { return transactionId; }
    @NonNull public String getAuthCode() { return authCode; }
    @NonNull public String getResponseCode() { return responseCode; }
    @NonNull public String getMessage() { return message; }
    @NonNull public String getMaskedInstrument() { return maskedInstrument; }
    public long getLatencyMicros() { return latencyMicros; }
    public boolean isOfflineProcessed() { return offlineProcessed; }
}
