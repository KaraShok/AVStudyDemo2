//
// Created by KaraShokZ on 2022/10/11.
//

#ifndef AVSTUDYDEMO2_JAVACALLHELPER_H
#define AVSTUDYDEMO2_JAVACALLHELPER_H

#include <jni.h>
#include "macro.h"

class JavaCallHelper {
private:
    JavaVM *javaVm = 0;
    JNIEnv *jniEnv = 0;
    jobject instance;
    jmethodID onErrorId;
    jmethodID onPrepareId;
    jmethodID onProgressId;

public:
    JavaCallHelper(JavaVM *vm, JNIEnv *env, jobject jobj);

    ~JavaCallHelper();

    void onError(int thread, int code);

    void onPrepare(int thread);

    void onProgress(int thread, int progress);
};
#endif //AVSTUDYDEMO2_JAVACALLHELPER_H
