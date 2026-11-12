# Keep public SDK entry points and model classes
-keep public class com.omnipay.sdk.OmniPayClient {
    public *;
}
-keep public class com.omnipay.sdk.OmniPayConfig** {
    public *;
}
-keep public class com.omnipay.sdk.model.** {
    public *;
}

# Keep native JNI methods
-keepclasseswithmembernames class * {
    native <methods>;
}

# Keep crypto and biometric callback models
-keepclassmembers class * extends androidx.biometric.BiometricPrompt$AuthenticationCallback {
    public *;
}
