#include <jni.h>
#include <string>

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_hookso_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {
    std::string hello = "Hello from C++";

    return env->NewStringUTF(hello.c_str());
}
jstring getjClassName(JNIEnv* env,jclass class_one){
    jclass clsClazz = env->FindClass("java/lang/Class");
    jmethodID getNameMethod = env->GetMethodID(clsClazz, "getName", "()Ljava/lang/String;");
    jstring strObj = (jstring) env->CallObjectMethod(class_one, getNameMethod);
    const char *className = env->GetStringUTFChars(strObj, NULL);
    jstring result = env->NewStringUTF(className);
    env->ReleaseStringUTFChars(strObj, className);
    return result;
}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_example_hookso_MainActivity_retclassname(JNIEnv *env, jobject thiz, jclass name) {
    // TODO: implement retclassname()
    return getjClassName(env,name);
}