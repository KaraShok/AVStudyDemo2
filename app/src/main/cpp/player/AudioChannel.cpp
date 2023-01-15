//
// Created by KaraShokZ on 2022/10/11.
//

#include "AudioChannel.h"

extern "C" {
#include <libavutil/time.h>
}

AudioChannel::AudioChannel(int id, AVCodecContext *avCodecContext, AVRational rational, JavaCallHelper *javaCallHelper):
    BaseChannel(id,avCodecContext,rational,javaCallHelper) {
        outChannels = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);
        outSampleSize = av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);
        outSampleRate = 44100;

        data = static_cast<uint8_t *>(malloc(outSampleRate * outChannels * outSampleSize));
        memset(data,0,outSampleRate * outChannels * outSampleSize);
}

AudioChannel::~AudioChannel() {
    if(data){
        free(data);
        data = 0;
    }
}

void *task_decode(void *args) {
    AudioChannel *audioChannel = static_cast<AudioChannel *>(args);
    audioChannel->decode();
    return 0;
}

void *task_play(void *args) {
    AudioChannel *audioChannel = static_cast<AudioChannel *>(args);
    audioChannel->runPlay();
    return 0;
}

void AudioChannel::play() {
    LOGE("AudioChannel play");
    startQueueWork();
    //0+输出声道+输出采样位+输出采样率+  输入的3个参数
    swrContext = swr_alloc_set_opts(0, AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_S16, outSampleRate,
                                    avCodecContext->channel_layout, avCodecContext->sample_fmt,
                                    avCodecContext->sample_rate, 0, 0);
    //初始化
    swr_init(swrContext);
    isPlaying = 1;
    //1 、解码
    pthread_create(&pidDecode, 0, task_decode, this);
    //2、 播放
    pthread_create(&pidPlay, 0, task_play, this);
}

void AudioChannel::decode() {
    AVPacket *packet = 0;
    LOGE("AudioChannel decode start %d",isPlaying);
    while (isPlaying) {
        //取出一个数据包
        int ret = packets.pop(packet);
        if (!isPlaying) {
            break;
        }
        //取出失败
        if (!ret) {
            continue;
        }
        //把包丢给解码器
        ret = avcodec_send_packet(avCodecContext, packet);
        freeAVPacket(&packet);
        //重试
        if (ret != 0) {
            break;
        }
        //代表了一个图像 (将这个图像先输出来)
        AVFrame *frame = av_frame_alloc();
        //从解码器中读取 解码后的数据包 AVFrame
        ret = avcodec_receive_frame(avCodecContext, frame);
        //需要更多的数据才能够进行解码
        if (ret == AVERROR(EAGAIN)) {
            continue;
        } else if (ret != 0) {
            break;
        }
        while (frames.size() > 100 && isPlaying) {
            av_usleep(1000 * 10);
            continue;
        }
        //再开一个线程 来播放 (流畅度)
        frames.push(frame);
    }
    LOGE("AudioChannel play end");
    freeAVPacket(&packet);
}

void bqPlayerCallback(SLAndroidSimpleBufferQueueItf bq, void *context) {
    AudioChannel *audioChannel = static_cast<AudioChannel *>(context);
    //获得pcm 数据 多少个字节 data
    int dataSize = audioChannel->getPCM();
    if(dataSize > 0 ){
        // 接收16位数据
        (*bq)->Enqueue(bq,audioChannel->data,dataSize);
    }
}

void AudioChannel::runPlay() {
    LOGE("AudioChannel runPlay start");
    SLresult result;
    // 1.1 创建引擎 SLObjectItf engineObject
    result = slCreateEngine(&engineObject, 0, NULL, 0, NULL, NULL);
    if (SL_RESULT_SUCCESS != result) {
        return;
    }
    // 1.2 初始化引擎  init
    result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
    if (SL_RESULT_SUCCESS != result) {
        return;
    }
    // 1.3 获取引擎接口SLEngineItf engineInterface
    result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE,
                                           &engineInterface);
    if (SL_RESULT_SUCCESS != result) {
        return;
    }

    /**
     * 2、设置混音器
     */
    // 2.1 创建混音器SLObjectItf outputMixObject
    result = (*engineInterface)->CreateOutputMix(engineInterface, &outputMixObject, 0,
                                                 0, 0);
    if (SL_RESULT_SUCCESS != result) {
        return;
    }
    // 2.2 初始化混音器outputMixObject
    result = (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);
    if (SL_RESULT_SUCCESS != result) {
        return;
    }

    /**
     * 3、创建播放器
     */
    //3.1 配置输入声音信息
    //创建buffer缓冲类型的队列 2个队列
    SLDataLocator_AndroidSimpleBufferQueue android_queue = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,
                                                            2};
    //pcm数据格式
    //pcm+2(双声道)+44100(采样率)+ 16(采样位)+16(数据的大小)+LEFT|RIGHT(双声道)+小端数据
    SLDataFormat_PCM pcm = {SL_DATAFORMAT_PCM, 2, SL_SAMPLINGRATE_44_1, SL_PCMSAMPLEFORMAT_FIXED_16,
                            SL_PCMSAMPLEFORMAT_FIXED_16,
                            SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,
                            SL_BYTEORDER_LITTLEENDIAN};

    //数据源 将上述配置信息放到这个数据源中
    SLDataSource slDataSource = {&android_queue, &pcm};

    //3.2  配置音轨(输出)
    //设置混音器
    SLDataLocator_OutputMix outputMix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObject};
    SLDataSink audioSnk = {&outputMix, NULL};
    //需要的接口  操作队列的接口
    const SLInterfaceID ids[1] = {SL_IID_BUFFERQUEUE};
    const SLboolean req[1] = {SL_BOOLEAN_TRUE};
    //3.3 创建播放器
    (*engineInterface)->CreateAudioPlayer(engineInterface, &bqPlayerObject, &slDataSource,
                                          &audioSnk, 1,
                                          ids, req);
    //初始化播放器
    (*bqPlayerObject)->Realize(bqPlayerObject, SL_BOOLEAN_FALSE);

    //得到接口后调用  获取Player接口
    (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_PLAY, &bqPlayerInterface);


    /**
     * 4、设置播放回调函数
     */
    //获取播放器队列接口
    (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_BUFFERQUEUE,
                                    &bqPlayerBufferQueueItf);
    //设置回调
    (*bqPlayerBufferQueueItf)->RegisterCallback(bqPlayerBufferQueueItf,
                                                      bqPlayerCallback, this);
    /**
     * 5、设置播放状态
     */
    (*bqPlayerInterface)->SetPlayState(bqPlayerInterface, SL_PLAYSTATE_PLAYING);
    /**
     * 6、手动激活一下这个回调
     */
    bqPlayerCallback(bqPlayerBufferQueueItf, this);
}

int AudioChannel::getPCM() {
    int data_size = 0;
    AVFrame *frame;
    while (isPlaying) {
        int ret = frames.pop(frame);
        if (!isPlaying) {
            break;
        }
        if (!ret) {
            continue;
        }
        // 计算转换后的sample个数  类似 a * b / c
        // swr_get_delay： 延迟时间:  输入了10个 数据，可能这次转换只转换了8个数据，那么还剩余2个 这一个函数就是得到上一次剩余的这个2
        // av_rescale_rnd： 以3为单位的1 转为以2为单位
        // 10个2=20
        // 转成 以4 为单位
        // 10*4/2
        uint64_t dst_nb_samples = av_rescale_rnd(
                swr_get_delay(swrContext, frame->sample_rate) + frame->nb_samples,
                outSampleRate,
                frame->sample_rate,
                AV_ROUND_UP);
        // 转换，返回值为转换后的sample个数
        int nb = swr_convert(swrContext, &data, dst_nb_samples,
                             (const uint8_t **) frame->data, frame->nb_samples);
        //转换后多少数据
        data_size = nb * outChannels * outSampleSize;
        //音频的时间
        clock = frame->best_effort_timestamp * av_q2d(timeBase);
        if (javaCallHelper) {
            javaCallHelper->onProgress(THREAD_CHILD, clock);
        }
        break;
    }
    freeAVFrame(&frame);
    return data_size;
}

void AudioChannel::stop() {
    isPlaying = 0;
    javaCallHelper = 0;

    stopQueueWork();

    pthread_join(pidDecode,0);
    pthread_join(pidPlay,0);

    //设置停止状态
    if (bqPlayerInterface) {
        (*bqPlayerInterface)->SetPlayState(bqPlayerInterface, SL_PLAYSTATE_STOPPED);
        bqPlayerInterface = 0;
    }
    //销毁播放器
    if (bqPlayerObject) {
        (*bqPlayerObject)->Destroy(bqPlayerObject);
        bqPlayerObject = 0;
        bqPlayerBufferQueueItf = 0;
    }
    //销毁混音器
    if (outputMixObject) {
        (*outputMixObject)->Destroy(outputMixObject);
        outputMixObject = 0;
    }
    //销毁引擎
    if (engineObject) {
        (*engineObject)->Destroy(engineObject);
        engineObject = 0;
        engineInterface = 0;
    }

    if(swrContext){
        swr_free(&swrContext);
        swrContext = 0;
    }
}
