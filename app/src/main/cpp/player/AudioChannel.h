//
// Created by KaraShokZ on 2022/10/11.
//

#ifndef AVSTUDYDEMO2_AUDIOCHANNEL_H
#define AVSTUDYDEMO2_AUDIOCHANNEL_H

#include "BaseChannel.h"
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

extern  "C"{
#include <libswresample/swresample.h>
};

class AudioChannel: public BaseChannel {
private:
    pthread_t pidDecode;
    pthread_t pidPlay;

    SLObjectItf engineObject = 0;
    SLEngineItf engineInterface = 0;
    SLObjectItf outputMixObject = 0;
    SLObjectItf bqPlayerObject = 0;
    SLPlayItf  bqPlayerInterface = 0;
    SLAndroidSimpleBufferQueueItf bqPlayerBufferQueueItf = 0;
    SwrContext *swrContext = 0;

public:
    uint8_t *data = 0;
    int outChannels;
    int outSampleSize;
    int outSampleRate;
    double clock;

    AudioChannel(int id,AVCodecContext *avCodecContext, AVRational rational, JavaCallHelper *javaCallHelper);

    ~AudioChannel();

    void play();

    void stop();

    void decode();

    void runPlay();

    int getPCM();
};


#endif //AVSTUDYDEMO2_AUDIOCHANNEL_H
