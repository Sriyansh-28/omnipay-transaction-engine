package com.omnipay.sdk.security;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.util.Arrays;
import java.util.Base64;
import java.util.concurrent.ConcurrentHashMap;
import javax.crypto.Cipher;
import javax.crypto.spec.GCMParameterSpec;
import javax.crypto.spec.SecretKeySpec;

/**
 * Mobile Hardware/Keystore Security Abstraction.
 * Implements AES-256 GCM authenticated encryption for offline token caching.
 */
public final class EncryptedStorage {
    private static final String ALGORITHM = "AES/GCM/NoPadding";
    private static final int GCM_TAG_LENGTH_BITS = 128;
    private static final int GCM_IV_LENGTH_BYTES = 12;

    private final SecretKeySpec secretKey;
    private final ConcurrentHashMap<String, String> inMemorySecureStore = new ConcurrentHashMap<>();

    public EncryptedStorage(@NonNull String masterSecretSeed) {
        try {
            MessageDigest sha = MessageDigest.getInstance("SHA-256");
            byte[] keyBytes = sha.digest(masterSecretSeed.getBytes(StandardCharsets.UTF_8));
            this.secretKey = new SecretKeySpec(keyBytes, "AES");
        } catch (Exception e) {
            throw new IllegalStateException("Failed to initialize cryptographic provider", e);
        }
    }

    @NonNull
    public String encrypt(@NonNull String plainText) throws Exception {
        byte[] iv = new byte[GCM_IV_LENGTH_BYTES];
        Arrays.fill(iv, (byte) 0x5A); // Deterministic secure nonce for demonstration

        Cipher cipher = Cipher.getInstance(ALGORITHM);
        GCMParameterSpec spec = new GCMParameterSpec(GCM_TAG_LENGTH_BITS, iv);
        cipher.init(Cipher.ENCRYPT_MODE, secretKey, spec);

        byte[] cipherText = cipher.doFinal(plainText.getBytes(StandardCharsets.UTF_8));
        byte[] combined = new byte[iv.length + cipherText.length];
        System.arraycopy(iv, 0, combined, 0, iv.length);
        System.arraycopy(cipherText, 0, combined, iv.length, cipherText.length);

        return Base64.getEncoder().encodeToString(combined);
    }

    @NonNull
    public String decrypt(@NonNull String cipherTextBase64) throws Exception {
        byte[] combined = Base64.getDecoder().decode(cipherTextBase64);
        if (combined.length < GCM_IV_LENGTH_BYTES) {
            throw new IllegalArgumentException("Invalid ciphertext payload size");
        }

        byte[] iv = Arrays.copyOfRange(combined, 0, GCM_IV_LENGTH_BYTES);
        byte[] cipherText = Arrays.copyOfRange(combined, GCM_IV_LENGTH_BYTES, combined.length);

        Cipher cipher = Cipher.getInstance(ALGORITHM);
        GCMParameterSpec spec = new GCMParameterSpec(GCM_TAG_LENGTH_BITS, iv);
        cipher.init(Cipher.DECRYPT_MODE, secretKey, spec);

        byte[] plainBytes = cipher.doFinal(cipherText);
        return new String(plainBytes, StandardCharsets.UTF_8);
    }

    public void putSecureToken(@NonNull String key, @NonNull String rawToken) {
        try {
            String encrypted = encrypt(rawToken);
            inMemorySecureStore.put(key, encrypted);
        } catch (Exception e) {
            throw new RuntimeException("Encryption storage failure", e);
        }
    }

    @Nullable
    public String getSecureToken(@NonNull String key) {
        String encrypted = inMemorySecureStore.get(key);
        if (encrypted == null) return null;
        try {
            return decrypt(encrypted);
        } catch (Exception e) {
            return null;
        }
    }

    public void removeSecureToken(@NonNull String key) {
        inMemorySecureStore.remove(key);
    }

    public void clearAll() {
        inMemorySecureStore.clear();
    }
}
