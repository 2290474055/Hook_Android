//
// Created by wb.mzy on 2023/5/16.
//

#ifndef HOOKSO_MYHOOK_H
#define HOOKSO_MYHOOK_H
#include <jni.h>
#include <iostream>
#include <unistd.h>
#include <android/log.h>
#include <dlfcn.h>
#include "dobby.h"
#include "common.h"
#endif //HOOKSO_MYHOOK_H

char* jstringToChar(JNIEnv* env, jstring jstr) {
    if (jstr == NULL) {
        return NULL;
    }
    const char* utf = env->GetStringUTFChars(jstr, NULL);
    if (utf == NULL) {
        return NULL;
    }
    size_t len = strlen(utf);
    char* buf = new char[len + 1];
    strncpy(buf, utf, len);
    buf[len] = '\0';
    //env->ReleaseStringUTFChars(jstr, utf);
    return buf;
}

char* getjClassName(JNIEnv* env,jclass class_one){
    //jclass clsClazz = env->FindClass("java/lang/Class");
//    jmethodID getNameMethod = env->GetMethodID(clsClazz, "getName", "()Ljava/lang/String;");
//    jstring strObj = (jstring) env->CallObjectMethod(class_one, getNameMethod);
//    const char *className = env->GetStringUTFChars(strObj, NULL);
//    return (char*)className;
    return NULL;
}

typedef void (*PVMRuntimesetProcessPackageName)(JNIEnv* env,jclass ATTRIBUTE_UNUSED,jstring java_package_name);
//在此函数获取包名，定向hook
void (*s_VMRuntime_setProcessPackageName)(JNIEnv* env,jclass ATTRIBUTE_UNUSED,jstring java_package_name);
//替换setProcessPackageName的函数
void x_VMRuntime_setProcessPackageName(JNIEnv* env,jclass ATTRIBUTE_UNUSED,jstring java_package_name){
    const char* package_name = env->GetStringUTFChars(java_package_name, NULL);
    LOGE("x_VMRuntime_setProcessPackageName package_name:%s",package_name);
    //这个注入位置，位于app函数之前
    if(strcmp(package_name,"com.fy.qqkp.new.mi") == 0){
        jclass cls = env->FindClass("java/lang/System");
        jmethodID mid = env->GetStaticMethodID(cls, "load", "(Ljava/lang/String;)V");
        jstring libName = env->NewStringUTF("/data/local/tmp/libpatch_arm.so");
        env->CallStaticVoidMethod(cls, mid, libName);
        env->DeleteLocalRef(libName);
    }
    s_VMRuntime_setProcessPackageName(env,ATTRIBUTE_UNUSED,java_package_name);
    return;
}

typedef jstring (*PRuntimeNativeLoad)(JNIEnv* env, jclass ignored, jstring javaFilename,jobject javaLoader);
//原nativeLoad地址本地加载so
jstring (*s_runtime_nativeLoad)(JNIEnv* env, jclass ignored, jstring javaFilename,jobject javaLoader);
//替换nativeLoad的函数
jstring x_runtime_nativeLoad(JNIEnv* env, jclass ignored, jstring javaFilename,jobject javaLoader){
    char * java_file_name = jstringToChar(env,  javaFilename);
    LOGE("x_runtime_nativeLoad java_file_name:%s",java_file_name);
    if(strstr(java_file_name,"libSecShell.so")){//此处判断时机，在此so加载之前将自己的so加载进去
        char patch_java_file_name[] = "/data/local/tmp/libpatch_arm64.so";
        if(access(patch_java_file_name,F_OK) == 0){//判断该文件是否存在
            jstring jpatch_java_file_name = env->NewStringUTF(patch_java_file_name);
            LOGE("x_runtime_nativeLoad libpatch_arm64 cunzai");
        }
    }

    if(java_file_name != NULL){
        free(java_file_name);
    }
    //将原来的so也加载进内存
    jstring jret = s_runtime_nativeLoad(env,  ignored, javaFilename, javaLoader);
    return jret;
}

typedef jobject (*PRuntimeOpenDexFileNative)(JNIEnv* env,jclass,jstring javaSourceName,jstring javaOutputName,jint flags,jobject class_loader,jobjectArray dex_elements);
//原openDexFileNative地址本地加载so
jobject (*s_runtime_openDexFileNative)(JNIEnv* env,jclass,jstring javaSourceName,jstring javaOutputName,jint flags,jobject class_loader,jobjectArray dex_elements);
//替换openDexFileNative的函数
jobject x_runtime_openDexFileNative(JNIEnv* env,jclass class_,jstring javaSourceName,jstring javaOutputName,jint flags,jobject class_loader,jobjectArray dex_elements){
    char* cjavaSourceName = jstringToChar(env,javaSourceName);
    char* cjavaOutputName = jstringToChar(env,javaOutputName);
    LOGE("x_runtime_openDexFileNative javaSourceName:%s,javaOutputName:%s",cjavaSourceName,cjavaOutputName);
    jobject jstr =  s_runtime_openDexFileNative(env,class_,javaSourceName,javaOutputName,flags,class_loader,dex_elements);
    return jstr;
}

int (*s_jniRegisterNativeMethods)(JNIEnv* env, const char* className,const JNINativeMethod* methods, int method_count);
int x_jniRegisterNativeMethods(JNIEnv* env, const char* className,const JNINativeMethod* methods, int method_count){
    //LOGE("x_jniRegisterNativeMethods className:%s",className);
    JNINativeMethod * new_methods = NULL;
    //通过该注册函数获取Runtime函数中注册的nativeLoad地址
    if(strcmp("java/lang/Runtime",className) == 0){
        new_methods = new JNINativeMethod[method_count];
        memcpy(new_methods, methods, sizeof(JNINativeMethod) * method_count);
        const JNINativeMethod  * method = NULL;
        for (int i = 0; i < method_count; i++) {
            method = &methods[i];
            //此函数用于加载本地so
            //LOGE("x_jniRegisterNativeMethods Runtime method:%s",method->name);
            if((strcmp("nativeLoad",method->name) == 0)){
                //拿到原始nativeLoad地址
                s_runtime_nativeLoad = (PRuntimeNativeLoad)new_methods[i].fnPtr;
                //LOGE("s_runtime_nativeLoad:%p",s_runtime_nativeLoad);
                //将自己的nativeLoad替换进去
                new_methods[i].fnPtr = (void *)&x_runtime_nativeLoad;
                LOGE("-->%s%s:%p",methods[i].name,methods[i].signature,methods[i].fnPtr);
            }
        }
    }
    jint ret =  s_jniRegisterNativeMethods(env, className, new_methods == NULL ? methods : new_methods, method_count);
    if(new_methods != NULL){
        delete[] new_methods;
    }
    return ret;
}

//原RegisterNativeMethods地址
jint (*s_art_RegisterNatives)(JNIEnv* env, jclass java_class, const JNINativeMethod* methods,jint method_count);
//替换RegisterNativeMethods的函数
jint x_art_RegisterNatives(JNIEnv* env, jclass java_class, const JNINativeMethod* methods,jint method_count){
    JNINativeMethod * new_methods = NULL;
    new_methods = new JNINativeMethod[method_count];

//    char* ClassName = getjClassName(env,java_class);
//    LOGE("x_art_RegisterNatives ClassName:%s",ClassName);
    //目的是因为参数methods为const属性
    memcpy(new_methods, methods, sizeof(JNINativeMethod) * method_count);
    for (int i = 0; i < method_count; i++) {
        if(strcmp(new_methods[i].name,"setProcessPackageName") == 0){
            //拿到原始setProcessPackageName地址
            s_VMRuntime_setProcessPackageName = (PVMRuntimesetProcessPackageName)new_methods[i].fnPtr;
            //LOGE("s_runtime_nativeLoad:%p",s_runtime_nativeLoad);
            //将自己的setProcessPackageName替换进去
            new_methods[i].fnPtr = (void *)&x_VMRuntime_setProcessPackageName;
            LOGE("-->%s%s:%p",methods[i].name,methods[i].signature,methods[i].fnPtr);
        }
        if(strcmp(new_methods[i].name,"openDexFileNative") == 0){
            //LOGE("x_art_RegisterNatives openDexFileNative:yes");

            //拿到原始openDexFileNative地址
            s_runtime_openDexFileNative = (PRuntimeOpenDexFileNative)new_methods[i].fnPtr;
            //LOGE("s_runtime_nativeLoad:%p",s_runtime_nativeLoad);
            //将自己的openDexFileNative替换进去
            new_methods[i].fnPtr = (void *)&x_runtime_openDexFileNative;
            LOGE("-->%s%s:%p",methods[i].name,methods[i].signature,methods[i].fnPtr);
        }
    }

    //将method注册
    jint jret = s_art_RegisterNatives(env, java_class, new_methods, method_count);
    if(new_methods != NULL){
        delete[] new_methods;
    }
    return jret;
}

