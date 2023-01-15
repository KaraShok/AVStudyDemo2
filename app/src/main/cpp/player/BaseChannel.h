//
// Created by KaraShokZ on 2022/10/11.
//

#ifndef AVSTUDYDEMO2_BASECHANNEL_H
#define AVSTUDYDEMO2_BASECHANNEL_H

#include "../safe_queue.h"
#include "JavaCallHelper.h"

extern "C" {
#include <libavcodec/avcodec.h>
};

class BaseChannel {
public:
    volatile int channelId;
    int isPlaying;
    double clock = 0;
    AVCodecContext *avCodecContext;
    SafeQueue<AVPacket*> packets;
    SafeQueue<AVFrame*> frames;
    AVRational timeBase;
    JavaCallHelper *javaCallHelper;

    BaseChannel(int id, AVCodecContext *context, AVRational rational, JavaCallHelper *javaCallHelper):
    channelId(id),
    avCodecContext(context),
    timeBase(rational),
    javaCallHelper(javaCallHelper) {
        packets.setReleaseCallback(freeAVPacket);
        frames.setReleaseCallback(freeAVFrame);
    }

    ~BaseChannel() {
        if (avCodecContext) {
            avcodec_close(avCodecContext);
            avcodec_free_context(&avCodecContext);
            avCodecContext = 0;
        }
        packets.clear();
        frames.clear();
    }

    virtual void play() = 0;

    virtual void stop() = 0;

    static void freeAVPacket(AVPacket **packet) {
        if (packet) {
            av_packet_free(packet);
            packet = 0;
        }
    }

    static void freeAVFrame(AVFrame **frame) {
        if (frame) {
            av_frame_free(frame);
            frame = 0;
        }
    }

    void clearQueue() {
        packets.clear();
        frames.clear();
    }

    void stopQueueWork() {
        packets.setWork(0);
        frames.setWork(0);
    }

    void startQueueWork() {
        packets.setWork(1);
        frames.setWork(1);
    }
};
#endif //AVSTUDYDEMO2_BASECHANNEL_H
