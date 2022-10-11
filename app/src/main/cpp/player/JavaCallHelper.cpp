//
// Created by KaraShokZ on 2022/10/11.
//

#include "JavaCallHelper.h"

JavaCallHelper::JavaCallHelper(JavaVM *vm, JNIEnv *env, jobject jobj) {
    javaVm = vm;
    jniEnv = env;
    instance = env->NewGlobalRef(jobj);

    jclass jcls = env->GetObjectClass(jobj);
    onErrorId = env->GetMethodID(jcls,"onError","(I)V");
    onPrepareId = env->GetMethodID(jcls,"onPrepare","()V");
}

JavaCallHelper::~JavaCallHelper() {
    jniEnv->DeleteGlobalRef(instance);
}

void JavaCallHelper::onError(int thread, int code) {
    if (thread == THREAD_MAIN) {
        jniEnv->CallVoidMethod(instance,onErrorId,code);
    } else {
        JNIEnv *env;
        javaVm->AttachCurrentThread(&env,0);
        env->CallVoidMethod(instance,onErrorId,code);
        javaVm->DetachCurrentThread();
    }
}

void JavaCallHelper::onPrepare(int thread) {
    if (thread == THREAD_MAIN) {
        jniEnv->CallVoidMethod(instance,onPrepareId);
    } else {
        JNIEnv *env;
        javaVm->AttachCurrentThread(&env,0);
        env->CallVoidMethod(instance,onPrepareId);
        javaVm->DetachCurrentThread();
    }
}
