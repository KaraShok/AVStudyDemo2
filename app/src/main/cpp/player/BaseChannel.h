//
// Created by KaraShokZ on 2022/10/11.
//

#ifndef AVSTUDYDEMO2_BASECHANNEL_H
#define AVSTUDYDEMO2_BASECHANNEL_H

#include "../safe_queue.h"

extern "C" {
#include <libavcodec/avcodec.h>
};

class BaseChannel {
public:
    int id;
    int isPlaying;
    AVCodecContext *avCodecContext;
    SafeQueue<AVPacket*> packets;
    SafeQueue<AVFrame*> frames;
    AVRational timeBase;

    BaseChannel(int id, AVCodecContext *context,AVRational rational):
        id(id),
        avCodecContext(context),
        timeBase(rational) {

    }

    ~BaseChannel() {
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
};
#endif //AVSTUDYDEMO2_BASECHANNEL_H
