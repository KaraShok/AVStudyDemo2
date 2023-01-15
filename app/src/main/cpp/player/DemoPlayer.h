//
// Created by KaraShokZ on 2022/10/11.
//

#ifndef AVSTUDYDEMO2_DEMOPLAYER_H
#define AVSTUDYDEMO2_DEMOPLAYER_H

#include "JavaCallHelper.h"
#include "AudioChannel.h"
#include "VideoChannel.h"
#include <cstring>
#include <pthread.h>
#include "macro.h"

extern  "C" {
#include <libavformat/avformat.h>
}

class DemoPlayer {
private:
    char *videoUrl = 0;
    int isSeek = 0;
    int duration = 0;
    pthread_t pidStop;
    pthread_mutex_t seekMutex;
    JavaCallHelper *javaCallHelper = 0;
    RenderFrameCallback  renderFrameCallback;

public:

    int isPlaying;
    pthread_t pidPrepare;
    pthread_t pidStart;
    AudioChannel *audioChannel = 0;
    VideoChannel *videoChannel = 0;
    AVFormatContext *avFormatContext = 0;

    DemoPlayer(JavaCallHelper *helper, const char *url);

    ~DemoPlayer();

    void prepare();

    void start();

    void stop();

    void runPrepare();

    void runStart();

    void setRenderFrameCallback(RenderFrameCallback callback);

    int getDuration() {
        return duration;
    }

    void seek(int i);
};
#endif //AVSTUDYDEMO2_DEMOPLAYER_H
