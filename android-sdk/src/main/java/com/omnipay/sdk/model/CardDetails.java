package com.omnipay.sdk.model;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

/**
 * Encapsulates payment card details with client-side Luhn Mod-10 validation
 * and sensitive field masking.
 */
public final class CardDetails {
    private final String cardNumber;
    private final String cardholderName;
    private final int expiryMonth;
    private final int expiryYear;
    private final String cvv;
    private final String billingZip;

    public CardDetails(@NonNull String cardNumber,
                       @NonNull String cardholderName,
                       int expiryMonth,
                       int expiryYear,
                       @NonNull String cvv,
                       @Nullable String billingZip) {
        this.cardNumber = sanitizeCardNumber(cardNumber);
        this.cardholderName = cardholderName.trim();
        this.expiryMonth = expiryMonth;
        this.expiryYear = expiryYear;
        this.cvv = cvv.trim();
        this.billingZip = billingZip != null ? billingZip.trim() : "";
    }

    public static String sanitizeCardNumber(String raw) {
        if (raw == null) return "";
        return raw.replaceAll("[\\s-]", "");
    }

    public boolean isValidLuhn() {
        if (cardNumber.length() < 13 || cardNumber.length() > 19) {
            return false;
        }
        int sum = 0;
        boolean alternate = false;
        for (int i = cardNumber.length() - 1; i >= 0; i--) {
            int n = cardNumber.charAt(i) - '0';
            if (n < 0 || n > 9) return false;
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

    public boolean isValidExpiry() {
        if (expiryMonth < 1 || expiryMonth > 12) return false;
        return expiryYear >= 2024 && expiryYear <= 2045;
    }

    public boolean isValidCvv() {
        int len = cvv.length();
        return (len == 3 || len == 4) && cvv.matches("\\d+");
    }

    public boolean isValid() {
        return isValidLuhn() && isValidExpiry() && isValidCvv() && !cardholderName.isEmpty();
    }

    @NonNull
    public String getCardBrand() {
        if (cardNumber.startsWith("4")) return "VISA";
        if (cardNumber.startsWith("34") || cardNumber.startsWith("37")) return "AMEX";
        if (cardNumber.startsWith("51") || cardNumber.startsWith("52") ||
            cardNumber.startsWith("53") || cardNumber.startsWith("54") ||
            cardNumber.startsWith("55")) return "MASTERCARD";
        if (cardNumber.startsWith("6011") || cardNumber.startsWith("65")) return "DISCOVER";
        return "UNKNOWN";
    }

    @NonNull
    public String getMaskedCardNumber() {
        if (cardNumber.length() < 10) return "XXXX-****";
        String first4 = cardNumber.substring(0, 4);
        String last4 = cardNumber.substring(cardNumber.length() - 4);
        return first4 + "-XXXX-XXXX-" + last4;
    }

    @NonNull
    public String getCardNumber() { return cardNumber; }
    @NonNull
    public String getCardholderName() { return cardholderName; }
    public int getExpiryMonth() { return expiryMonth; }
    public int getExpiryYear() { return expiryYear; }
    @NonNull
    public String getCvv() { return cvv; }
    @NonNull
    public String getBillingZip() { return billingZip; }
}
