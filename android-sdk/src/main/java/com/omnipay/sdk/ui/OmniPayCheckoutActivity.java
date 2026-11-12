package com.omnipay.sdk.ui;

import android.os.Bundle;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.RadioGroup;
import android.widget.Toast;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;

import com.google.android.material.textfield.TextInputLayout;
import com.omnipay.sdk.OmniPayClient;
import com.omnipay.sdk.R;
import com.omnipay.sdk.model.CardDetails;
import com.omnipay.sdk.model.PaymentRequest;
import com.omnipay.sdk.model.PaymentResponse;

/**
 * Mobile Checkout Activity implementing responsive design,
 * WCAG 2.1 AA accessibility guidelines, dynamic card brand detection,
 * inline error feedback, and offline transaction handling.
 */
public class OmniPayCheckoutActivity extends AppCompatActivity {

    private LinearLayout bannerOfflineNotice;
    private RadioGroup rgPaymentMethod;
    private TextInputLayout tilCardNumber;
    private TextInputLayout tilCardholderName;
    private TextInputLayout tilExpiry;
    private TextInputLayout tilCvv;
    private EditText etCardNumber;
    private EditText etCardholderName;
    private EditText etExpiry;
    private EditText etCvv;
    private Button btnPayNow;

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_omnipay_checkout);

        bindViews();
        setupInputWatchers();
        setupPaymentMethodSelector();
        observeNetworkState();
        setupSubmissionHandler();
    }

    private void bindViews() {
        bannerOfflineNotice = findViewById(R.id.banner_offline_notice);
        rgPaymentMethod = findViewById(R.id.rg_payment_method);
        tilCardNumber = findViewById(R.id.til_card_number);
        tilCardholderName = findViewById(R.id.til_cardholder_name);
        tilExpiry = findViewById(R.id.til_expiry);
        tilCvv = findViewById(R.id.til_cvv);
        etCardNumber = findViewById(R.id.et_card_number);
        etCardholderName = findViewById(R.id.et_cardholder_name);
        etExpiry = findViewById(R.id.et_expiry);
        etCvv = findViewById(R.id.et_cvv);
        btnPayNow = findViewById(R.id.btn_pay_now);
    }

    private void setupInputWatchers() {
        // Card number format and brand detection
        etCardNumber.addTextChangedListener(new TextWatcher() {
            @Override public void beforeTextChanged(CharSequence s, int start, int count, int after) {}
            @Override public void onTextChanged(CharSequence s, int start, int before, int count) {}
            @Override public void afterTextChanged(Editable s) {
                String clean = CardDetails.sanitizeCardNumber(s.toString());
                if (clean.length() >= 4) {
                    if (clean.startsWith("4")) {
                        tilCardNumber.setEndIconContentDescription("Visa card detected");
                    } else if (clean.startsWith("51") || clean.startsWith("55")) {
                        tilCardNumber.setEndIconContentDescription("Mastercard detected");
                    } else if (clean.startsWith("34") || clean.startsWith("37")) {
                        tilCardNumber.setEndIconContentDescription("American Express detected");
                    }
                }
                tilCardNumber.setError(null);
            }
        });

        // Expiry Date Auto-slash insertion (MM/YY)
        etExpiry.addTextChangedListener(new TextWatcher() {
            private boolean isFormatting = false;
            @Override public void beforeTextChanged(CharSequence s, int start, int count, int after) {}
            @Override public void onTextChanged(CharSequence s, int start, int before, int count) {}
            @Override public void afterTextChanged(Editable s) {
                if (isFormatting) return;
                isFormatting = true;
                String str = s.toString().replaceAll("[^\\d]", "");
                if (str.length() >= 2) {
                    String month = str.substring(0, 2);
                    String year = str.substring(2);
                    String formatted = month + "/" + year;
                    etExpiry.setText(formatted);
                    etExpiry.setSelection(Math.min(formatted.length(), 5));
                }
                isFormatting = false;
                tilExpiry.setError(null);
            }
        });
    }

    private void setupPaymentMethodSelector() {
        rgPaymentMethod.setOnCheckedChangeListener((group, checkedId) -> {
            if (checkedId == R.id.rb_method_card) {
                tilCardNumber.setVisibility(View.VISIBLE);
                tilCardholderName.setVisibility(View.VISIBLE);
                tilExpiry.setVisibility(View.VISIBLE);
                tilCvv.setVisibility(View.VISIBLE);
                btnPayNow.setText(R.string.btn_pay_now);
            } else if (checkedId == R.id.rb_method_nfc) {
                tilCardNumber.setVisibility(View.GONE);
                tilCardholderName.setVisibility(View.GONE);
                tilExpiry.setVisibility(View.GONE);
                tilCvv.setVisibility(View.GONE);
                btnPayNow.setText("Hold Device Near Terminal");
            } else if (checkedId == R.id.rb_method_qr) {
                tilCardNumber.setVisibility(View.GONE);
                tilCardholderName.setVisibility(View.GONE);
                tilExpiry.setVisibility(View.GONE);
                tilCvv.setVisibility(View.GONE);
                btnPayNow.setText("Scan Merchant QR Code");
            }
        });
    }

    private void observeNetworkState() {
        try {
            OmniPayClient client = OmniPayClient.getInstance();
            client.getNetworkMonitor().addListener(isConnected -> runOnUiThread(() -> {
                bannerOfflineNotice.setVisibility(isConnected ? View.GONE : View.VISIBLE);
            }));
        } catch (Exception ignored) {}
    }

    private void setupSubmissionHandler() {
        btnPayNow.setOnClickListener(v -> {
            int selectedMethod = rgPaymentMethod.getCheckedRadioButtonId();

            if (selectedMethod == R.id.rb_method_card) {
                validateAndSubmitCardPayment();
            } else if (selectedMethod == R.id.rb_method_nfc) {
                submitNfcPayment();
            } else {
                submitQrPayment();
            }
        });
    }

    private void validateAndSubmitCardPayment() {
        String pan = etCardNumber.getText().toString().trim();
        String name = etCardholderName.getText().toString().trim();
        String exp = etExpiry.getText().toString().trim();
        String cvv = etCvv.getText().toString().trim();

        int month = 0, year = 0;
        if (exp.contains("/")) {
            String[] parts = exp.split("/");
            try {
                month = Integer.parseInt(parts[0]);
                year = 2000 + Integer.parseInt(parts[1]);
            } catch (Exception ignored) {}
        }

        CardDetails card = new CardDetails(pan, name, month, year, cvv, "94105");

        if (!card.isValid()) {
            if (!card.isValidLuhn()) tilCardNumber.setError("Invalid card number checksum");
            if (name.isEmpty()) tilCardholderName.setError("Name is required");
            if (!card.isValidExpiry()) tilExpiry.setError("Invalid date");
            if (!card.isValidCvv()) tilCvv.setError("Invalid CVV");
            return;
        }

        btnPayNow.setEnabled(false);
        btnPayNow.setText("Securing Transaction...");

        PaymentRequest req = PaymentRequest.createCardPayment(49.99, "USD", "MERCH-001", "CUST-99", card);
        OmniPayClient.getInstance().authorizeTransaction(req, new OmniPayClient.PaymentCallback() {
            @Override
            public void onCompleted(@NonNull PaymentResponse response) {
                runOnUiThread(() -> {
                    btnPayNow.setEnabled(true);
                    btnPayNow.setText(R.string.btn_pay_now);
                    String msg = response.isOfflineProcessed() ?
                        "Offline Queued: " + response.getTransactionId() :
                        "Approved! Auth Code: " + response.getAuthCode();
                    Toast.makeText(OmniPayCheckoutActivity.this, msg, Toast.LENGTH_LONG).show();
                    finish();
                });
            }

            @Override
            public void onError(@NonNull String errorCode, @NonNull String errorMessage) {
                runOnUiThread(() -> {
                    btnPayNow.setEnabled(true);
                    btnPayNow.setText(R.string.btn_pay_now);
                    Toast.makeText(OmniPayCheckoutActivity.this, "Payment Failed (" + errorCode + "): " + errorMessage, Toast.LENGTH_LONG).show();
                });
            }
        });
    }

    private void submitNfcPayment() {
        PaymentRequest req = PaymentRequest.createNfcPayment(49.99, "USD", "MERCH-001", "CUST-99", "NFC_TOKEN_EMV");
        OmniPayClient.getInstance().authorizeTransaction(req, new OmniPayClient.PaymentCallback() {
            @Override
            public void onCompleted(@NonNull PaymentResponse response) {
                runOnUiThread(() -> {
                    Toast.makeText(OmniPayCheckoutActivity.this, "NFC Payment Authorized: " + response.getAuthCode(), Toast.LENGTH_SHORT).show();
                    finish();
                });
            }
            @Override public void onError(@NonNull String errorCode, @NonNull String errorMessage) {}
        });
    }

    private void submitQrPayment() {
        PaymentRequest req = PaymentRequest.createQrPayment(49.99, "USD", "MERCH-001", "CUST-99", "000201010212...EMVCO");
        OmniPayClient.getInstance().authorizeTransaction(req, new OmniPayClient.PaymentCallback() {
            @Override
            public void onCompleted(@NonNull PaymentResponse response) {
                runOnUiThread(() -> {
                    Toast.makeText(OmniPayCheckoutActivity.this, "QR Payment Authorized: " + response.getAuthCode(), Toast.LENGTH_SHORT).show();
                    finish();
                });
            }
            @Override public void onError(@NonNull String errorCode, @NonNull String errorMessage) {}
        });
    }
}
