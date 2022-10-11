//
// Created by KaraShokZ on 2022/10/11.
//

#ifndef AVSTUDYDEMO2_VIDEOCHANNEL_H
#define AVSTUDYDEMO2_VIDEOCHANNEL_H

#include "BaseChannel.h"

extern "C" {
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
};

typedef void (*RenderFrameCallback)(uint8_t *,int,int,int);

class VideoChannel: public BaseChannel {
private:
    pthread_t pidDecode;
    pthread_t pidRender;
    RenderFrameCallback renderFrameCallback;
    SwsContext *swsContext = 0;

public:
    VideoChannel(int id, AVCodecContext *context);

    ~VideoChannel();

    void play();

    void decode();

    void render();

    void setRenderFrameCallback(RenderFrameCallback callback);
};
#endif //AVSTUDYDEMO2_VIDEOCHANNEL_H
