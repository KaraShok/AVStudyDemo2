//
// Created by KaraShokZ on 2022/10/11.
//

#include "VideoChannel.h"

void dropAVFrame(queue<AVFrame*> &q) {
    if (!q.empty()) {
        AVFrame *frame = q.front();
        BaseChannel::freeAVFrame(&frame);
        q.pop();
    }
}

VideoChannel::VideoChannel(int id, AVCodecContext *context, AVRational rational,int fps, JavaCallHelper *javaCallHelper):
    BaseChannel(id, context, rational, javaCallHelper) {
    this->fps = fps;
    frames.setSyncHandle(dropAVFrame);
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
    startQueueWork();

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
        if (res == AVERROR(EAGAIN)) {
            //需要更多数据
            continue;
        } else if (res < 0) {
            //失败
            break;
        }

        AVFrame *frame = av_frame_alloc();
        res = avcodec_receive_frame(avCodecContext,frame);
        if (res == AVERROR(EAGAIN)) {
            //需要更多数据
            continue;
        } else if (res < 0) {
            break;
        }
        while (frames.size() > 100 && isPlaying) {
            av_usleep(1000 * 10);
            continue;
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
        if (!res) {
            continue;
        }

        res = syncAudioAndVideo(frame);

        if (!res) {
            continue;
        }

        //diff太大了不回调了
        if (javaCallHelper && !audioChannel) {
            javaCallHelper->onProgress(THREAD_CHILD, clock);
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

    isPlaying = 0;

    av_freep(&dstData[0]);
    freeAVFrame(&frame);

    sws_freeContext(swsContext);
    swsContext = 0;
}

void VideoChannel::setRenderFrameCallback(RenderFrameCallback callback) {
    renderFrameCallback = callback;
}

void VideoChannel::setAudioChannel(AudioChannel *channel) {
    audioChannel = channel;
}

int VideoChannel::syncAudioAndVideo(AVFrame *frame) {
    /**
     * seek需要注意的点：编码器中存在缓存,100s 的图像,用户seek到第 50s 的位置
     * 音频是50s的音频，但是视频 你获得的是100s的视频
     */
    int res = 1;
    double frameDelays = 1.0 / fps;
    double extraDelay = frame->repeat_pict / (2 * fps);
    double delays = extraDelay + frameDelays;
    if ((clock = frame->best_effort_timestamp) == AV_NOPTS_VALUE) {
        clock = 0;
    }
    //pts 单位就是time_base
    //av_q2d转为双精度浮点数 乘以 pts 得到pts --- 显示时间:秒
    clock = clock * av_q2d(timeBase);
    if (clock == 0) {
        //正常播放
        av_usleep(delays * 1000000);
    } else {
        double audioClock = audioChannel ? audioChannel->clock : 0;
        double diff = fabs(clock - audioClock);
        LOGE("当前和音频比较:%f - %f = %f", clock, audioClock, diff);
        //允许误差 diff > 0.04 &&
        if (audioChannel) {
            //如果视频比音频快，延迟差值播放，否则直接播放
            if (clock > audioClock) {
                if (diff > 1) {
                    //差的太久了， 那只能慢慢赶 不然就是卡好久
                    av_usleep((delays * 2) * 1000000);
                } else {
                    //差的不多，尝试一次赶上去
                    av_usleep((delays + diff) * 1000000);
                }
            } else {
                //音频比视频快
                //视频慢了 0.05s 已经比较明显了 (丢帧)
                if (diff > 1) {
                    //一种可能： 快进了(因为解码器中有缓存数据，这样获得的avframe就和seek的匹配了)
                } else if (diff >= 0.05) {
                    freeAVFrame(&frame);
                    //执行同步操作 删除到最近的key frame
                    frames.sync();
                    res = 0;
                } else {
                    //不休眠 加快速度赶上去
                }
            }
        } else {
            //正常播放
            av_usleep(delays * 1000000);
        }
    }
    return res;
}

void VideoChannel::stop() {
    isPlaying = 0;
    javaCallHelper = 0;
    stopQueueWork();
    pthread_join(pidRender,0);
    pthread_join(pidDecode,0);

}