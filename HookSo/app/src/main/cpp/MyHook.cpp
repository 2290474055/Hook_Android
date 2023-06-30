//
// Created by wb.mzy on 2023/5/13.
//
#include "MyHook.h"
__attribute__ ((constructor(101))) static void so_init(void);

void so_init(void)
{
    LOGE("welcome so_init");

    void * handle = dlopen("libart.so",0);
    //void * ptr_art_RegisterNativeMethods = dlsym(handle, "_ZN3art3JNI15RegisterNativesEP7_JNIEnvP7_jclassPK15JNINativeMethodi");

    void * ptr_art_RegisterNatives = DobbySymbolResolver("libart.so","_ZN3art3JNI15RegisterNativesEP7_JNIEnvP7_jclassPK15JNINativeMethodi");
    LOGE("ptr_art_RegisterNativeMethods %x:",ptr_art_RegisterNatives);
    DobbyHook(reinterpret_cast< void *>(ptr_art_RegisterNatives), reinterpret_cast< void *>(&x_art_RegisterNatives), reinterpret_cast< void **>(&s_art_RegisterNatives));

    //hook jniRegisterNativeMethods函数来获取nativeLoad函数
    DobbyHook(reinterpret_cast< void *>(DobbySymbolResolver("libnativehelper.so","jniRegisterNativeMethods")),//函数地址
              reinterpret_cast< void *>(&x_jniRegisterNativeMethods),//替代函数
              reinterpret_cast< void **>(&s_jniRegisterNativeMethods));//原始函数指针
}


