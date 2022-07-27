
#ifndef PUSHDEMO_VIDEOCHANNEL_H
#define PUSHDEMO_VIDEOCHANNEL_H

#include "x264.h"
#include <pty.h>
#include <cstring>
#include "librtmp/rtmp.h"
#include "macro.h"

class VideoChannel {
    typedef void (*VideoCallback)(RTMPPacket *packet);

public:
    void setVideoEncInfo(int width, int height, int fps, int bitrate);

    void setVideoCallback(VideoCallback videoCallback_);

    void encodeData(int8_t *data);

    ~VideoChannel();

private:
    int mWidth;
    int mHeight;
    int mFps;
    int mBitrate;
    int ySize;   //yuv y分量长度
    int uvSize;  //yuv uv分量长度
    x264_t *videoCodec;
    x264_picture_t *pic_in; // 一帧的数据
    VideoCallback videoCallback;

    void sendSpsPps(uint8_t sps[100], uint8_t pps[100], int sps_len, int pps_len);

    void sendFrame(int type, uint8_t *payload, int i_payload);
};

#endif //PUSHDEMO_VIDEOCHANNEL_H
