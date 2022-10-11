//
// Created by KaraShokZ on 2022/10/11.
//

#include "VideoChannel.h"

VideoChannel::VideoChannel(int id, AVCodecContext *context) : BaseChannel(id, context) {

}

VideoChannel::~VideoChannel() {

}

void *taskDecode(void *args) {
    VideoChannel *channel = static_cast<VideoChannel *>(args);
    channel->decode();
    return 0;
}

void *taskRender(void *args) {
    VideoChannel *channel = static_cast<VideoChannel *>(args);
    channel->render();
    return 0;
}

void VideoChannel::play() {
    isPlaying = 1;
    frames.setWork(1);
    packets.setWork(1);

    pthread_create(&pidDecode,0,taskDecode,this);

    pthread_create(&pidRender,0,taskRender,this);
}

void VideoChannel::decode() {
    AVPacket *packet = 0;
    while (isPlaying) {
        int res = packets.pop(packet);

        if (!isPlaying) {
            break;
        }

        if (!res) {
            continue;
        }

        res = avcodec_send_packet(avCodecContext,packet);
        freeAVPacket(&packet);
        if (res != 0) {
            break;
        }

        AVFrame *frame = av_frame_alloc();
        res = avcodec_receive_frame(avCodecContext,frame);
        if (res == AVERROR(EAGAIN)) {
            continue;
        } else if(res != 0){
            break;
        }

        frames.push(frame);
    }
    freeAVPacket(&packet);
}

void VideoChannel::render() {
    swsContext = sws_getContext(
            avCodecContext->width,avCodecContext->height,avCodecContext->pix_fmt,
            avCodecContext->width,avCodecContext->height,AV_PIX_FMT_RGBA,
            SWS_BILINEAR,0,0,0
            );

    AVFrame *frame = 0;
    uint8_t *dstData[4];
    int dstLineSize[4];

    av_image_alloc(
            dstData,
            dstLineSize,
            avCodecContext->width,
            avCodecContext->height,
            AV_PIX_FMT_RGBA,
            1
            );

    while (isPlaying) {
        int res = frames.pop(frame);
        if (!isPlaying) {
            break;
        }

        sws_scale(
                swsContext,
                frame->data,
                frame->linesize,
                0,
                avCodecContext->height,
                dstData,
                dstLineSize
                );

        renderFrameCallback(
                dstData[0],
                dstLineSize[0],
                avCodecContext->width,
                avCodecContext->height
                );

        freeAVFrame(&frame);
    }
    av_freep(&dstData[0]);
    freeAVFrame(&frame);
}

void VideoChannel::setRenderFrameCallback(RenderFrameCallback callback) {
    renderFrameCallback = callback;
}
