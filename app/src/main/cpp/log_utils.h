//
// Created by KaraShokZ on 2022/9/24.
//

#ifndef VIDEOSTUDYDEMO_LOG_UTILS_H
#define VIDEOSTUDYDEMO_LOG_UTILS_H

#include <android/log.h>

#define LOGI(TAG,FORMAT,...) __android_log_print(ANDROID_LOG_INFO,TAG,FORMAT,##__VA_ARGS__);
#define LOGE(TAG,FORMAT,...) __android_log_print(ANDROID_LOG_ERROR,TAG,FORMAT,##__VA_ARGS__);

#endif //VIDEOSTUDYDEMO_LOG_UTILS_H
