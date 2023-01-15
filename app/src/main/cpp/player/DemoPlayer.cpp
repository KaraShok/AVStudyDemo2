//
// Created by KaraShokZ on 2022/10/11.
//

#include "DemoPlayer.h"

DemoPlayer::DemoPlayer(JavaCallHelper *helper, const char *url) {
    javaCallHelper = helper;
    videoUrl = new char[strlen(url) + 1];
    strcpy(videoUrl,url);
    isPlaying = 0;
    duration = 0;
    pthread_mutex_init(&seekMutex,0);
}

DemoPlayer::~DemoPlayer() {
    DELETE(videoUrl);
    DELETE(javaCallHelper);
    videoUrl = 0;
    javaCallHelper = 0;
    pthread_mutex_destroy(&seekMutex);
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
    avFormatContext = avformat_alloc_context();

    //1、打开URL
    AVDictionary *opts = NULL;
    //设置超时3秒
    av_dict_set(&opts, "timeout", "3000000", 0);

    int res = avformat_open_input(&avFormatContext,videoUrl,0,&opts);

    if (res != 0) {
        LOGE("打开媒体失败:%s", av_err2str(res));
        if (javaCallHelper) {
            javaCallHelper->onError(THREAD_CHILD, FFMPEG_CAN_NOT_OPEN_URL);
        }
        return;
    }
    res = avformat_find_stream_info(avFormatContext,0);
    if (res < 0) {
        LOGE("查找流失败:%s", av_err2str(res));
        if (javaCallHelper) {
            javaCallHelper->onError(THREAD_CHILD, FFMPEG_CAN_NOT_FIND_STREAMS);
        }
        return;
    }
    duration = avFormatContext->duration / 1000000;
    for (int i = 0; i < avFormatContext->nb_streams; ++i) {
        AVStream *stream = avFormatContext->streams[i];
        AVCodecParameters *parameters = stream->codecpar;
        AVCodec *codec = avcodec_find_decoder(parameters->codec_id);
        if (codec == NULL) {
            LOGE("查找解码器失败:%s", av_err2str(res));
            if (javaCallHelper) {
                javaCallHelper->onError(THREAD_CHILD, FFMPEG_FIND_DECODER_FAIL);
            }
            return;
        }
        AVCodecContext *context = avcodec_alloc_context3(codec);
        if (context == NULL) {
            LOGE("创建解码上下文失败:%s", av_err2str(res));
            if (javaCallHelper) {
                javaCallHelper->onError(THREAD_CHILD, FFMPEG_ALLOC_CODEC_CONTEXT_FAIL);
            }
            return;
        }
        res = avcodec_parameters_to_context(context,parameters);
        if (res < 0) {
            LOGE("设置解码上下文参数失败:%s", av_err2str(res));
            if (javaCallHelper) {
                javaCallHelper->onError(THREAD_CHILD, FFMPEG_CODEC_CONTEXT_PARAMETERS_FAIL);
            }
            return;
        }
        res = avcodec_open2(context,codec,0);
        if (res != 0) {
            LOGE("打开解码器失败:%s", av_err2str(res));
            if (javaCallHelper) {
                javaCallHelper->onError(THREAD_CHILD, FFMPEG_OPEN_DECODER_FAIL);
            }
            return;
        }

        AVRational timeBase = stream->time_base;

        if (parameters->codec_type == AVMEDIA_TYPE_AUDIO) {
            audioChannel = new AudioChannel(i,context,timeBase,javaCallHelper);
        } else if (parameters->codec_type == AVMEDIA_TYPE_VIDEO) {
            AVRational frameRate = stream->avg_frame_rate;
            int fps = av_q2d(frameRate);

            videoChannel = new VideoChannel(i,context,timeBase,fps,javaCallHelper);
            videoChannel->setRenderFrameCallback(renderFrameCallback);
        }
    }

    if (!audioChannel && !videoChannel) {
        if (javaCallHelper) {
            javaCallHelper->onError(THREAD_CHILD, FFMPEG_NOMEDIA);
        }
        return;
    }
    // 准备完了 通知java 你随时可以开始播放
    if (javaCallHelper) {
        javaCallHelper->onPrepare(THREAD_CHILD);
    }
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

        pthread_mutex_lock(&seekMutex);
        AVPacket *packet = av_packet_alloc();
        ret = av_read_frame(avFormatContext, packet);
        pthread_mutex_unlock(&seekMutex);
        //=0成功 其他:失败
        if (ret == 0) {
            //stream_index 这一个流的一个序号
            if (audioChannel && packet->stream_index == audioChannel->channelId) {
                audioChannel->packets.push(packet);
            } else
            if (videoChannel && packet->stream_index == videoChannel->channelId) {
                videoChannel->packets.push(packet);
            }
        } else if (ret == AVERROR_EOF) {
            //读取完成 但是可能还没播放完
            if (audioChannel->packets.empty() && audioChannel->frames.empty()
                && videoChannel->packets.empty() && videoChannel->frames.empty()) {
                LOGE("播放完毕");
                break;
            }
        } else {
            //
        }
    }
    isPlaying = 0;
    if (audioChannel) {
        audioChannel->stop();
//        DELETE(audioChannel);
    }
    if (videoChannel) {
        videoChannel->stop();
//        DELETE(videoChannel);
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
    DemoPlayer* player = static_cast<DemoPlayer*>(args);
    player->isPlaying = 0;
    if (player->pidPrepare) {
        pthread_join(player->pidPrepare,0);
    }
    if (player->pidStart) {
        pthread_join(player->pidStart,0);
    }
    DELETE(player->videoChannel);
    DELETE(player->audioChannel);
    if (player->avFormatContext) {
        avformat_close_input(&player->avFormatContext);
        avformat_free_context(player->avFormatContext);
        player->avFormatContext = 0;
    }
    DELETE(player);
    return 0;
}

void DemoPlayer::stop() {
    javaCallHelper = 0;
    isPlaying = 0;
//    if (audioChannel) {
//        audioChannel->stop();
//
//    }
//    if (videoChannel) {
//        videoChannel->stop();
//    }
    pthread_create(&pidStop,0,taskStop,this);
}

void DemoPlayer::seek(int i) {
//进去必须 在0- duration 范围之类
    if (i< 0 || i >= duration) {
        return;
    }
    if (!audioChannel && !videoChannel) {
        return;
    }
    if (!avFormatContext) {
        return;
    }
    isSeek = 1;
    pthread_mutex_lock(&seekMutex);
    //单位是 微妙
    int64_t seek = i * 1000000;
    //seek到请求的时间 之前最近的关键帧
    // 只有从关键帧才能开始解码出完整图片
    av_seek_frame(avFormatContext, -1,seek, AVSEEK_FLAG_BACKWARD);
//    avformat_seek_file(formatContext, -1, INT64_MIN, seek, INT64_MAX, 0);
    // 音频、与视频队列中的数据 是不是就可以丢掉了？
    if (audioChannel) {
        //暂停队列
        audioChannel->stopQueueWork();
        //可以清空缓存
//        avcodec_flush_buffers();
        audioChannel->clearQueue();
        //启动队列
        audioChannel->startQueueWork();
    }
    if (videoChannel) {
        videoChannel->stopQueueWork();
        videoChannel->clearQueue();
        videoChannel->startQueueWork();
    }
    pthread_mutex_unlock(&seekMutex);
    isSeek = 0;
}