#include <string>
#include <sstream>
#include <memory>
#include <map>
#include "omnipay/engine/TransactionEngine.hpp"
#include "omnipay/instrument/PaymentInstrumentFactory.hpp"

#ifdef __ANDROID__
#include <jni.h>
#else
// Standalone mock typedefs for JNI compilation on non-Android platforms
typedef void* jobject;
typedef void* jclass;
typedef const char* jstring;
typedef struct JNIEnv {
    const char* (*GetStringUTFChars)(struct JNIEnv*, jstring, unsigned char*);
    void (*ReleaseStringUTFChars)(struct JNIEnv*, jstring, const char*);
    jstring (*NewStringUTF)(struct JNIEnv*, const char*);
} JNIEnv;
#define JNIEXPORT
#define JNICALL
#endif

// Global single engine instance for mobile client JNI
static std::unique_ptr<omnipay::TransactionEngine> g_mobileEngine = nullptr;

extern "C" {

JNIEXPORT jstring JNICALL
Java_com_omnipay_sdk_OmniPayClient_nativeGetCoreVersion(JNIEnv* env, jobject /* this */) {
    const char* version = "OmniPay-Core-C++17-v2.4.0-Enterprise";
#ifdef __ANDROID__
    return env->NewStringUTF(version);
#else
    return version;
#endif
}

JNIEXPORT jstring JNICALL
Java_com_omnipay_sdk_OmniPayClient_nativeAuthorizeTransaction(
    JNIEnv* env, jobject /* this */, jstring payloadJson) {
    
    if (!g_mobileEngine) {
        omnipay::TransactionEngine::EngineConfig cfg;
        cfg.workerThreads = 2; // Lightweight on mobile devices
        g_mobileEngine = std::make_unique<omnipay::TransactionEngine>(cfg);
    }

    // Process payment through C++ core engine
    omnipay::TransactionContext ctx;
    ctx.transactionId = "TX-MOB-NATIVE-8901";
    ctx.accountId = "ACC-MOB-4111";
    ctx.amount = 49.99;
    ctx.currency = "USD";
    ctx.location = {37.7749, -122.4194, "US", "San Francisco"};

    auto card = omnipay::PaymentInstrumentFactory::createCreditCard(
        "4111111111111111", "Alex Morgan", 12, 2028, "123", "94105", true);

    omnipay::TransactionResult res = g_mobileEngine->processTransaction(ctx, *card);

    std::stringstream ss;
    ss << "{\"status\":\"" << omnipay::transactionStatusToString(res.status) << "\""
       << ",\"authCode\":\"" << res.authorizationCode << "\""
       << ",\"responseCode\":\"" << res.responseCode << "\""
       << ",\"responseMessage\":\"" << res.responseMessage << "\""
       << ",\"latencyMicros\":" << res.latencyMicros << "}";
    
    std::string jsonStr = ss.str();

#ifdef __ANDROID__
    return env->NewStringUTF(jsonStr.c_str());
#else
    return jsonStr.c_str();
#endif
}

JNIEXPORT jstring JNICALL
Java_com_omnipay_sdk_OmniPayClient_nativeEvaluateRisk(
    JNIEnv* env, jobject /* this */, jstring contextJson) {
    
    omnipay::VelocityCheckStrategy velocity;
    omnipay::TransactionContext ctx;
    ctx.transactionId = "TX-RISK-CHECK";
    ctx.accountId = "ACC-USER-1";
    ctx.amount = 100.0;

    auto eval = velocity.evaluate(ctx);

    std::stringstream ss;
    ss << "{\"score\":" << eval.score
       << ",\"action\":\"" << omnipay::riskActionToString(eval.action) << "\""
       << ",\"evaluationNotes\":\"" << eval.evaluationNotes << "\"}";
    
    std::string jsonStr = ss.str();

#ifdef __ANDROID__
    return env->NewStringUTF(jsonStr.c_str());
#else
    return jsonStr.c_str();
#endif
}

} // extern "C"
