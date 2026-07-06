#include <jni.h>
#include <dlfcn.h>
#include <string>
#include <android/log.h>

#define LOG_TAG "AmneziaJNI"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// Function pointer types matching libawg_proxy.so exports
typedef int (*StartAWGProxyFunc)(const char* uapi, const char* localIp, const char* dnsIp, int port);
typedef int (*StopAWGProxyFunc)();

static void* g_libHandle = nullptr;
static StartAWGProxyFunc g_startFunc = nullptr;
static StopAWGProxyFunc g_stopFunc = nullptr;

extern "C" {

JNIEXPORT jint JNICALL
Java_org_telegram_messenger_AmneziaManager_nativeLoadLibrary(JNIEnv* env, jobject thiz) {
    if (g_libHandle != nullptr) {
        LOGI("Library already loaded");
        return 0;
    }

    g_libHandle = dlopen("libawg_proxy.so", RTLD_NOW);
    if (!g_libHandle) {
        LOGE("Failed to load libawg_proxy.so: %s", dlerror());
        return -1;
    }

    g_startFunc = (StartAWGProxyFunc)dlsym(g_libHandle, "StartAWGProxy");
    if (!g_startFunc) {
        LOGE("Failed to find StartAWGProxy: %s", dlerror());
        dlclose(g_libHandle);
        g_libHandle = nullptr;
        return -2;
    }

    g_stopFunc = (StopAWGProxyFunc)dlsym(g_libHandle, "StopAWGProxy");
    if (!g_stopFunc) {
        LOGE("Failed to find StopAWGProxy: %s", dlerror());
        dlclose(g_libHandle);
        g_libHandle = nullptr;
        return -3;
    }

    LOGI("libawg_proxy.so loaded successfully");
    return 0;
}

JNIEXPORT jint JNICALL
Java_org_telegram_messenger_AmneziaManager_nativeStartProxy(
    JNIEnv* env, jobject thiz,
    jstring uapi, jstring localIp, jstring dnsIp, jint port) {

    if (!g_startFunc) {
        LOGE("Library not loaded");
        return -100;
    }

    const char* uapiStr = env->GetStringUTFChars(uapi, nullptr);
    const char* localIpStr = env->GetStringUTFChars(localIp, nullptr);
    const char* dnsIpStr = env->GetStringUTFChars(dnsIp, nullptr);

    LOGI("Starting AWG proxy on port %d", port);
    int result = g_startFunc(uapiStr, localIpStr, dnsIpStr, port);

    env->ReleaseStringUTFChars(uapi, uapiStr);
    env->ReleaseStringUTFChars(localIp, localIpStr);
    env->ReleaseStringUTFChars(dnsIp, dnsIpStr);

    LOGI("StartAWGProxy returned: %d", result);
    return result;
}

JNIEXPORT jint JNICALL
Java_org_telegram_messenger_AmneziaManager_nativeStopProxy(JNIEnv* env, jobject thiz) {
    if (!g_stopFunc) {
        LOGE("Library not loaded");
        return -100;
    }

    LOGI("Stopping AWG proxy");
    int result = g_stopFunc();
    LOGI("StopAWGProxy returned: %d", result);
    return result;
}

JNIEXPORT jboolean JNICALL
Java_org_telegram_messenger_AmneziaManager_nativeIsLoaded(JNIEnv* env, jobject thiz) {
    return g_libHandle != nullptr && g_startFunc != nullptr && g_stopFunc != nullptr;
}

} // extern "C"
