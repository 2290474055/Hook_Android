//
// Created by wb.mzy on 2023/5/15.
//
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include<sys/types.h>
#include<sys/stat.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <jni.h>
#include <utility>
#include <vector>
#include <iostream>
#include <string>
#include "common.h"
#include <sys/prctl.h>
#include "dobby.h"
#include "common.h"


char * get_current_threadname() {
    //int pthread_getname_np(pthread_t *thread,
    //                       const char *name, size_t len);
    char tname[0x100] = {0};
    prctl(PR_GET_NAME, tname);
    return strdup(tname);
}


int  (*s_open)(const char *pathname, int flags, mode_t mode);
int  x_open(const char *pathname, int flags, mode_t mode){
    char* ThreadName = get_current_threadname();
    LOGE("open filename:%s flags:%d tname:%s",pathname,flags,ThreadName);
    return s_open(pathname,flags,mode);
}

jint  (*s_RegisterNatives)(JNIEnv*env,jclass clazz, const JNINativeMethod* methods,jint nMethods);
jint  x_RegisterNatives(JNIEnv*env,jclass clazz, const JNINativeMethod* methods,jint nMethods){
    extern int errno;
    for(int i = 0;i< nMethods;i++){
        LOGE("RegisterNatives name:%s signature:%s fnPtr:%llx",methods[i].name,methods[i].signature,methods[i].fnPtr);
        if((strstr(methods[i].name,"getobjresult")) != NULL){
//            LOGE("start dump mem");
//            void* startAddress = (void*)((long long)methods[i].fnPtr & (long long)0xfffffffffffff000);
//            LOGE("write startAddress:%llx",startAddress);
//            int fn = open("/data/data/com.qcplay.smjh.qcplay/dumpMem",O_CREAT|O_RDWR,0777);
//            if(fn < 0){
//                LOGE("open dump fail errno:%d",errno);
//            }else{
//                ssize_t sSize = write(fn,startAddress,0x1000);
//                if(sSize < 0){
//                    LOGE("write fail errno:%d",errno);
//                }
//                close(fn);
//            }
        }
    }
    return s_RegisterNatives(env,clazz,methods,nMethods);
}

jint  (*s_FindClass)(JNIEnv*env,const char* name);
jint  x_FindClass(JNIEnv*env,const char* name){
    LOGE("FindClass name:%s",name);
    return s_FindClass(env,name);
}

jint  (*s_GetMethodID)(JNIEnv*env,jclass clazz, const char* name, const char* sig);
jint  x_GetMethodID(JNIEnv*env,jclass clazz, const char* name, const char* sig){
    LOGE("GetMethodID name:%s",name);
    return s_GetMethodID(env,clazz,name,sig);
}

jint  (*s_GetFieldID)(JNIEnv*env,jclass clazz, const char* name, const char* sig);
jint  x_GetFieldID(JNIEnv*env,jclass clazz, const char* name, const char* sig){
    LOGE("GetFieldID name:%s",name);
    return s_GetFieldID(env,clazz,name,sig);
}

jint  (*s_SetObjectField)(JNIEnv*env,jobject obj, jfieldID fieldID, jobject value);
jint  x_SetObjectField(JNIEnv*env,jobject obj, jfieldID fieldID, jobject value){
    LOGE("SetObjectField jobject:%d",obj);
    return s_SetObjectField(env,obj,fieldID,value);
}

//jint  (*s_CallStaticObjectMethod)(JNIEnv* env, jclass clazz, jmethodID methodID, ...);
//jint  x_CallStaticObjectMethod(JNIEnv* env, jclass clazz, jmethodID methodID, ...){
//    LOGE("SetObjectField jobject:%d",obj);
//    return s_CallStaticObjectMethod(env,obj,fieldID,value);
//}
//
//jint  (*s_CallIntMethod)(JNIEnv*env,jobject obj, jfieldID fieldID, jobject value);
//jint  x_CallIntMethod(JNIEnv*env,jobject obj, jfieldID fieldID, jobject value){
//    LOGE("SetObjectField jobject:%d",obj);
//    return s_CallIntMethod(env,obj,fieldID,value);
//}

jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    LOGE("patch_arm64 JNI_OnLoad");

    JNIEnv* env = NULL;
    vm->GetEnv((void**)&env,JNI_VERSION_1_6);

    pid_t mId = getpid();
    LOGE("patch_arm64 JNI_OnLoad mId:%d",mId);
    DobbyHook(reinterpret_cast< void *>(env->functions->RegisterNatives), reinterpret_cast< void *>(&x_RegisterNatives),reinterpret_cast< void **>(&s_RegisterNatives));
    DobbyHook(reinterpret_cast< void *>(env->functions->FindClass), reinterpret_cast< void *>(&x_FindClass),reinterpret_cast< void **>(&s_FindClass));
    DobbyHook(reinterpret_cast< void *>(env->functions->GetMethodID), reinterpret_cast< void *>(&x_GetMethodID),reinterpret_cast< void **>(&s_GetMethodID));
    DobbyHook(reinterpret_cast< void *>(env->functions->GetFieldID), reinterpret_cast< void *>(&x_GetFieldID),reinterpret_cast< void **>(&s_GetFieldID));
    DobbyHook(reinterpret_cast< void *>(env->functions->SetObjectField), reinterpret_cast< void *>(&x_SetObjectField),reinterpret_cast< void **>(&s_SetObjectField));
    DobbyHook(reinterpret_cast< void *>(env->functions->CallStaticObjectMethod), reinterpret_cast< void *>(&x_SetObjectField),reinterpret_cast< void **>(&s_SetObjectField));
    DobbyHook(reinterpret_cast< void *>(env->functions->CallIntMethod), reinterpret_cast< void *>(&x_SetObjectField),reinterpret_cast< void **>(&s_SetObjectField));
    //hook自己想要的代码
    DobbyHook(DobbySymbolResolver("libc.so", "open"), reinterpret_cast< void *>(&x_open),
              reinterpret_cast< void **>(&s_open));
    return JNI_VERSION_1_6;
}