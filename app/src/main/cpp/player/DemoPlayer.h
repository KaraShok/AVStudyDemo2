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
    int isPlaying;
    pthread_t pidPrepare;
    pthread_t pidStart;
    AVFormatContext *avFormatContext = 0;
    JavaCallHelper *javaCallHelper = 0;
    AudioChannel *audioChannel = 0;
    VideoChannel *videoChannel = 0;
    RenderFrameCallback  renderFrameCallback;



public:
    DemoPlayer(JavaCallHelper *helper, const char *url);

    ~DemoPlayer();

    void prepare();

    void start();

    void runPrepare();

    void runStart();

    void setRenderFrameCallback(RenderFrameCallback callback);
};
#endif //AVSTUDYDEMO2_DEMOPLAYER_H
