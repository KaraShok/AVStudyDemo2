//
// Created by KaraShokZ on 2022/10/11.
//

#include "DemoPlayer.h"

DemoPlayer::DemoPlayer(JavaCallHelper *helper, const char *url) {
    javaCallHelper = helper;
    videoUrl = new char[strlen(url) + 1];
    strcpy(videoUrl,url);
}

DemoPlayer::~DemoPlayer() {
    DELETE(videoUrl);
    DELETE(javaCallHelper);
}

void* taskPrepare(void *args) {
    DemoPlayer *player = static_cast<DemoPlayer *>(args);
    player->runPrepare();
    return 0;
}

void DemoPlayer::prepare() {
    pthread_create(&pidPrepare,0,taskPrepare,this);
}

void DemoPlayer::runPrepare() {
    avformat_network_init();
    avFormatContext = 0;
    int res = avformat_open_input(&avFormatContext,videoUrl,0,0);
    if (res != 0) {
        LOGE("打开媒体失败:%s", av_err2str(res));
        javaCallHelper->onError(THREAD_CHILD, FFMPEG_CAN_NOT_OPEN_URL);
        return;
    }
    res = avformat_find_stream_info(avFormatContext,0);
    if (res < 0) {
        LOGE("查找流失败:%s", av_err2str(res));
        javaCallHelper->onError(THREAD_CHILD, FFMPEG_CAN_NOT_FIND_STREAMS);
        return;
    }
    for (int i = 0; i < avFormatContext->nb_streams; ++i) {
        AVStream *stream = avFormatContext->streams[i];
        AVCodecParameters *parameters = stream->codecpar;
        AVCodec *codec = avcodec_find_decoder(parameters->codec_id);
        if (codec == NULL) {
            LOGE("查找解码器失败:%s", av_err2str(res));
            javaCallHelper->onError(THREAD_CHILD, FFMPEG_FIND_DECODER_FAIL);
            return;
        }
        AVCodecContext *context = avcodec_alloc_context3(codec);
        if (context == NULL) {
            LOGE("创建解码上下文失败:%s", av_err2str(res));
            javaCallHelper->onError(THREAD_CHILD, FFMPEG_ALLOC_CODEC_CONTEXT_FAIL);
            return;
        }
        res = avcodec_parameters_to_context(context,parameters);
        if (res < 0) {
            LOGE("设置解码上下文参数失败:%s", av_err2str(res));
            javaCallHelper->onError(THREAD_CHILD, FFMPEG_CODEC_CONTEXT_PARAMETERS_FAIL);
            return;
        }
        res = avcodec_open2(context,codec,0);
        if (res != 0) {
            LOGE("打开解码器失败:%s", av_err2str(res));
            javaCallHelper->onError(THREAD_CHILD, FFMPEG_OPEN_DECODER_FAIL);
            return;
        }

        AVRational timeBase = stream->time_base;

        if (parameters->codec_type == AVMEDIA_TYPE_AUDIO) {
            audioChannel = new AudioChannel(i,context,timeBase);
        } else if (parameters->codec_type == AVMEDIA_TYPE_VIDEO) {
            AVRational frameRate = stream->avg_frame_rate;
            int fps = av_q2d(frameRate);

            videoChannel = new VideoChannel(i,context,timeBase,fps);
            videoChannel->setRenderFrameCallback(renderFrameCallback);
        }
    }

    if (!audioChannel && !videoChannel) {
        LOGE("没有音视频");
        javaCallHelper->onError(THREAD_CHILD, FFMPEG_NOMEDIA);
        return;
    }
    // 准备完了 通知java 你随时可以开始播放
    javaCallHelper->onPrepare(THREAD_CHILD);
}

void DemoPlayer::runStart() {
    int ret;
    while (isPlaying) {

        if (audioChannel && audioChannel->packets.size() > 100) {
            av_usleep(1000 * 10);
            continue;
        }

        if (videoChannel && videoChannel->packets.size() > 100) {
            av_usleep(1000 * 10);
            continue;
        }

        AVPacket *packet = av_packet_alloc();
        ret = av_read_frame(avFormatContext, packet);
        //=0成功 其他:失败
        if (ret == 0) {
            //stream_index 这一个流的一个序号
            if (audioChannel && packet->stream_index == audioChannel->id) {
                audioChannel->packets.push(packet);
            } else
            if (videoChannel && packet->stream_index == videoChannel->id) {
                videoChannel->packets.push(packet);
            }
        } else if (ret == AVERROR_EOF) {
            //读取完成 但是可能还没播放完
            if (audioChannel->packets.empty() && audioChannel->frames.empty()
                && videoChannel->packets.empty() && videoChannel->frames.empty()) {
                break;
            }
        } else {
            //
        }
    }
}

void* taskStart(void *args) {
    DemoPlayer *player = static_cast<DemoPlayer *>(args);
    player->runStart();
    return 0;
}

void DemoPlayer::start() {
    isPlaying = 1;

    if (audioChannel) {
        audioChannel->play();
    }

    if (videoChannel) {
        videoChannel->setAudioChannel(audioChannel);
        videoChannel->play();
    }

    pthread_create(&pidStart,0,taskStart, this);
}

void DemoPlayer::setRenderFrameCallback(RenderFrameCallback callback) {
    renderFrameCallback = callback;
}

void *taskStop(void *args) {

    return 0;
}

void DemoPlayer::stop() {

}