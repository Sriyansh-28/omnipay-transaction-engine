package com.omnipay.sdk;

import androidx.annotation.NonNull;

/**
 * Mobile SDK Configuration Builder.
 */
public final class OmniPayConfig {
    public enum Environment {
        SANDBOX,
        STAGING,
        PRODUCTION
    }

    private final String publishableApiKey;
    private final String merchantId;
    private final Environment environment;
    private final boolean offlineCachingEnabled;
    private final int requestTimeoutMs;
    private final boolean biometricRequired;

    private OmniPayConfig(Builder builder) {
        this.publishableApiKey = builder.publishableApiKey;
        this.merchantId = builder.merchantId;
        this.environment = builder.environment;
        this.offlineCachingEnabled = builder.offlineCachingEnabled;
        this.requestTimeoutMs = builder.requestTimeoutMs;
        this.biometricRequired = builder.biometricRequired;
    }

    @NonNull public String getPublishableApiKey() { return publishableApiKey; }
    @NonNull public String getMerchantId() { return merchantId; }
    @NonNull public Environment getEnvironment() { return environment; }
    public boolean isOfflineCachingEnabled() { return offlineCachingEnabled; }
    public int getRequestTimeoutMs() { return requestTimeoutMs; }
    public boolean isBiometricRequired() { return biometricRequired; }

    public static class Builder {
        private String publishableApiKey = "pk_live_omnipay_sample_key";
        private String merchantId = "merch_default_123";
        private Environment environment = Environment.PRODUCTION;
        private boolean offlineCachingEnabled = true;
        private int requestTimeoutMs = 5000;
        private boolean biometricRequired = false;

        public Builder setPublishableApiKey(@NonNull String apiKey) {
            this.publishableApiKey = apiKey;
            return this;
        }

        public Builder setMerchantId(@NonNull String merchantId) {
            this.merchantId = merchantId;
            return this;
        }

        public Builder setEnvironment(@NonNull Environment env) {
            this.environment = env;
            return this;
        }

        public Builder setOfflineCachingEnabled(boolean enabled) {
            this.offlineCachingEnabled = enabled;
            return this;
        }

        public Builder setRequestTimeoutMs(int timeoutMs) {
            this.requestTimeoutMs = timeoutMs;
            return this;
        }

        public Builder setBiometricRequired(boolean required) {
            this.biometricRequired = required;
            return this;
        }

        public OmniPayConfig build() {
            if (publishableApiKey == null || publishableApiKey.isEmpty()) {
                throw new IllegalArgumentException("Publishable API key is required");
            }
            return new OmniPayConfig(this);
        }
    }
}
