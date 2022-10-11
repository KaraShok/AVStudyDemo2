//
// Created by KaraShokZ on 2022/10/11.
//

#include <jni.h>
#include <string>
#include <android/native_window_jni.h>
#include "DemoPlayer.h"

DemoPlayer *player = 0;

JavaVM *javaVm = 0;
ANativeWindow *window = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER ;

int JNI_OnLoad(JavaVM *vm, void *r) {
    javaVm = vm;
    return JNI_VERSION_1_6;
}

void render(uint8_t *data, int lineszie, int w, int h) {
    pthread_mutex_lock(&mutex);
    if (!window) {
        pthread_mutex_unlock(&mutex);
        return;
    }
    //设置窗口属性
    ANativeWindow_setBuffersGeometry(window, w,
                                     h,
                                     WINDOW_FORMAT_RGBA_8888);

    ANativeWindow_Buffer window_buffer;
    if (ANativeWindow_lock(window, &window_buffer, 0)) {
        ANativeWindow_release(window);
        window = 0;
        pthread_mutex_unlock(&mutex);
        return;
    }
    //填充rgb数据给dst_data
    uint8_t *dst_data = static_cast<uint8_t *>(window_buffer.bits);
    // stride：一行多少个数据（RGBA） *4
    int dst_linesize = window_buffer.stride * 4;
    //一行一行的拷贝
    for (int i = 0; i < window_buffer.height; ++i) {
        //memcpy(dst_data , data, dst_linesize);
        memcpy(dst_data + i * dst_linesize, data + i * lineszie, dst_linesize);
    }
    ANativeWindow_unlockAndPost(window);
    pthread_mutex_unlock(&mutex);
}

extern "C" {

JNIEXPORT void JNICALL Java_com_example_avstudydemo2_player_DemoPlayer_nativePrepare
        (JNIEnv *env, jobject jobj, jstring url) {
    const char *videoUrl = env->GetStringUTFChars(url,0);
    JavaCallHelper *helper = new JavaCallHelper(javaVm,env,jobj);
    player = new DemoPlayer(helper,videoUrl);
    player->setRenderFrameCallback(render);
    player->prepare();
    env->ReleaseStringUTFChars(url,videoUrl);
}

JNIEXPORT void JNICALL Java_com_example_avstudydemo2_player_DemoPlayer_nativeStart
        (JNIEnv *, jobject) {
    player->start();
}

JNIEXPORT void JNICALL Java_com_example_avstudydemo2_player_DemoPlayer_nativeSetSurface
        (JNIEnv *env, jobject, jobject surface) {
    pthread_mutex_lock(&mutex);
    if (window) {
        ANativeWindow_release(window);
        window = 0;
    }
    window = ANativeWindow_fromSurface(env,surface);
    pthread_mutex_unlock(&mutex);
}

}