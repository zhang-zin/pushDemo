#include <jni.h>
#include <string>
#include <pthread.h>
#include <unistd.h>
#include "VideoChannel.h"
#include "AudioChannel.h"
#include "librtmp/rtmp.h"
#include "macro.h"
#include "safe_queue.h"

int isStart;
uint32_t start_time;
int readyPushing;
pthread_t pid;

VideoChannel *videoChannel;
AudioChannel *audioChannel;
SafeQueue<RTMPPacket *> packets; // 队列

void callback(RTMPPacket *packet) {
    if (packet) {
        // 设置时间戳
        packet->m_nTimeStamp = RTMP_GetTime() - start_time;
        // 加入队列
        packets.put(packet);
    }
}

void releasePackets(RTMPPacket *&packet) {
    if (packet) {
        RTMPPacket_Free(packet);
        delete packet;
        packet = nullptr;
    }
}

/**
 * 连接服务器
 * @param args
 * @return
 */
void *start(void *args) {
    char *url = static_cast<char *>(args);
    RTMP *rtmp;
    rtmp = RTMP_Alloc();
    if (!rtmp) {
        LOGE("alloc rtmp失败");
        return nullptr;
    }
    RTMP_Init(rtmp);
    int ret = RTMP_SetupURL(rtmp, url);
    if (!ret) {
        LOGE("设置地址失败:%s", url);
        return nullptr;
    }
    rtmp->Link.timeout = 5;
    RTMP_EnableWrite(rtmp);
    ret = RTMP_Connect(rtmp, nullptr);
    if (!ret) {
        LOGE("连接服务器:%s", url);
        return nullptr;
    }
    ret = RTMP_ConnectStream(rtmp, 0);
    if (!ret) {
        LOGE("连接流:%s", url);
        return nullptr;
    }
    // 表示可以开始推流了
    readyPushing = 1;
    start_time = RTMP_GetTime();

    packets.setWork(1);
    RTMPPacket *packet = nullptr;
    callback(audioChannel->getAudioTag());
    while (readyPushing) {
        packets.get(packet);
        if (!readyPushing) {
            break;
        }
        if (!packet) {
            continue;
        }
        LOGE("取出一帧数据");
        packet->m_nInfoField2 = rtmp->m_stream_id;
        ret = RTMP_SendPacket(rtmp, packet, 1);
        if (!ret) {
            LOGE("RTMP_SendPacket fail");
        }
        releasePackets(packet);
    }
    isStart = 0;
    readyPushing = 0;
    packets.setWork(0);
    packets.clear();
    if (rtmp) {
        RTMP_Close(rtmp);
        RTMP_Free(rtmp);
    }
    delete (url);
    return nullptr;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_zj26_pushdemo_LivePusher_native_1init(JNIEnv *env, jobject thiz) {
    videoChannel = new VideoChannel();
    videoChannel->setVideoCallback(callback);
    audioChannel = new AudioChannel();
    audioChannel->setAudioCallback(callback);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_zj26_pushdemo_LivePusher_native_1setVideoEncInfo(JNIEnv *env, jobject thiz, jint width,
                                                          jint height, jint fps, jint bitrate) {
    if (!videoChannel) {
        return;
    }
    videoChannel->setVideoEncInfo(width, height, fps, bitrate);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_zj26_pushdemo_LivePusher_native_1start(JNIEnv *env, jobject thiz, jstring path_) {
    if (isStart) {
        return;
    }
    isStart = 1;
    const char *path = env->GetStringUTFChars(path_, nullptr);
    char *url = new char[strlen(path) + 1];
    strcpy(url, path);
    pthread_create(&pid, nullptr, start, url);
    env->ReleaseStringUTFChars(path_, path);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_zj26_pushdemo_LivePusher_native_1pushVideo(JNIEnv *env, jobject thiz, jbyteArray data_) {
    if (!videoChannel || !readyPushing) {
        return;
    }
    jbyte *data = env->GetByteArrayElements(data_, nullptr);
    videoChannel->encodeData(data);
    env->ReleaseByteArrayElements(data_, data, 0);
}
extern "C"
JNIEXPORT jint JNICALL
Java_com_zj26_pushdemo_LivePusher_getInputSamples(JNIEnv *env, jobject thiz) {
    if (audioChannel) {
        return audioChannel->getInputSamples();
    }
    return -1;
}
extern "C"
JNIEXPORT void JNICALL
Java_com_zj26_pushdemo_LivePusher_native_1setAudioEncInfo(JNIEnv *env, jobject thiz,
                                                          jint sample_rate_in_hz, jint channel) {
    if (audioChannel) {
        audioChannel->setAudioEncInfo(sample_rate_in_hz, channel);
    }
}
extern "C"
JNIEXPORT void JNICALL
Java_com_zj26_pushdemo_LivePusher_native_1pushAudio(JNIEnv *env, jobject thiz, jbyteArray bytes) {
    if (!audioChannel || !readyPushing) {
        return;
    }
    jbyte *data = env->GetByteArrayElements(bytes, nullptr);
    audioChannel->encodeData(data);
    env->ReleaseByteArrayElements(bytes, data, 0);
}
extern "C"
JNIEXPORT void JNICALL
Java_com_zj26_pushdemo_LivePusher_release(JNIEnv *env, jobject thiz) {
    readyPushing = false;
    DELETE(audioChannel)
    DELETE(videoChannel)
}