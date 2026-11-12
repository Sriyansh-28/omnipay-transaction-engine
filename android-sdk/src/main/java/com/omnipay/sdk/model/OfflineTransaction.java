package com.omnipay.sdk.model;

import androidx.annotation.NonNull;

/**
 * Encapsulates an offline-queued transaction awaiting network synchronization.
 */
public final class OfflineTransaction {
    private final String queuedId;
    private final String transactionPayloadJson;
    private final long queuedTimestampMillis;
    private int retryCount;

    public OfflineTransaction(@NonNull String queuedId,
                              @NonNull String transactionPayloadJson,
                              long queuedTimestampMillis) {
        this.queuedId = queuedId;
        this.transactionPayloadJson = transactionPayloadJson;
        this.queuedTimestampMillis = queuedTimestampMillis;
        this.retryCount = 0;
    }

    @NonNull public String getQueuedId() { return queuedId; }
    @NonNull public String getTransactionPayloadJson() { return transactionPayloadJson; }
    public long getQueuedTimestampMillis() { return queuedTimestampMillis; }
    public int getRetryCount() { return retryCount; }
    public void incrementRetryCount() { this.retryCount++; }
}
