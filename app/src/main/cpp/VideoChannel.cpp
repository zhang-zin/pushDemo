#include "VideoChannel.h"

/**
 * 初始化视频编码信息
 * @param width
 * @param height
 * @param fps
 * @param bitrate
 */
void VideoChannel::setVideoEncInfo(int width, int height, int fps, int bitrate) {
    mWidth = width;
    mHeight = height;
    mFps = fps;
    mBitrate = bitrate;
    ySize = mWidth * mHeight;
    uvSize = ySize / 4;
    // 初始化x264编码器
    x264_param_t param;
    x264_param_default_preset(&param, "ultrafast", "zerolatency");
    // base_line 3.2 编码复杂度
    param.i_level_idc = 32;
    // 输入数据格式
    param.i_csp = X264_CSP_I420;
    param.i_width = width;
    param.i_height = height;
    param.i_bframe = 0; // 无b帧
    // 参数i_rc_method标识码率控制，CQP(恒定质量)，CRF(恒定码率)，ABR(平均码率)
    param.rc.i_rc_method = X264_RC_ABR;
    // 码率（比特率，单位kbps）
    param.rc.i_bitrate = mBitrate / 1000;
    // 瞬时最大码率
    param.rc.i_vbv_max_bitrate = mBitrate / 1000 * 1.2;
    // 设置了i_vbv_max_bitrate必须设置此参数，码率控制区大小,单位kbps
    param.rc.i_vbv_buffer_size = mBitrate / 1000;

    // 为了音视频同步
    param.i_fps_num = mFps;  // 帧率的分子
    param.i_fps_den = 1;     // 帧率的分母
    param.i_timebase_den = param.i_fps_num;  // 时间基准 分母
    param.i_timebase_num = param.i_fps_den;  // 时间基准 分子

    // VFR输入。1 ：时间基和时间戳用于码率控制  0 ：仅帧率用于码率控制
    param.b_vfr_input = 0;
    // 关键帧距离 2s一个关键帧
    param.i_keyint_max = fps * 2;
    // 是否复制sps和pps放在每个关键帧的前面
    param.b_repeat_headers = 1;
    // 多线程
    param.i_threads = 1;
    x264_param_apply_profile(&param, "baseline");
    videoCodec = x264_encoder_open(&param);
    pic_in = new x264_picture_t;
    x264_picture_alloc(pic_in, X264_CSP_I420, width, height);
}

void VideoChannel::setVideoCallback(VideoCallback videoCallback_) {
    this->videoCallback = videoCallback_;
}

/**
 * nv21 --> yuv420
 * @param date nv21数据
 */
void VideoChannel::encodeData(int8_t *data) {
    // 解码 nv21 -->  yuv420 数据：data，容器：pic_in
    // y数据 pic_in->img.plane
    memcpy(pic_in->img.plane[0], data, ySize);
    // uv数据
    for (int i = 0; i < uvSize; ++i) {
        *(pic_in->img.plane[1] + i) = *(data + ySize + i * 2 + 1);
        *(pic_in->img.plane[2] + i) = *(data + ySize + i * 2);
    }
    // NALU单元
    x264_nal_t *pp_nal;
    // 编码出有多少个数据 （多少NALU单元）
    int pi_nal;
    x264_picture_t pic_out;
    x264_encoder_encode(videoCodec, &pp_nal, &pi_nal, pic_in, &pic_out);
    int sps_len;
    int pps_len;
    uint8_t sps[100];
    uint8_t pps[100];
    for (int i = 0; i < pi_nal; ++i) {
        if (pp_nal[i].i_type == NAL_SPS) {
            sps_len = pp_nal[i].i_payload - 4;
            memcpy(sps, pp_nal[i].p_payload + 4, sps_len);
        } else if (pp_nal[i].i_type == NAL_PPS) {
            // 单独发送sps和pps
            pps_len = pp_nal[i].i_payload - 4;
            memcpy(pps, pp_nal[i].p_payload + 4, pps_len);
            sendSpsPps(sps, pps, sps_len, pps_len);
        } else {
            // 发送关键帧和非关键帧
            sendFrame(pp_nal[i].i_type, pp_nal[i].p_payload, pp_nal[i].i_payload);
        }
    }
}

void VideoChannel::sendSpsPps(uint8_t *sps, uint8_t *pps, int sps_len, int pps_len) {
    // sps,pps --->packet
    int bodySize = 13 + sps_len + 3 + pps_len;
    auto *packet = new RTMPPacket;
    RTMPPacket_Alloc(packet, bodySize);

    int i = 0;
    //固定头
    packet->m_body[i++] = 0x17;
    //类型
    packet->m_body[i++] = 0x00;
    //composition time 0x000000
    packet->m_body[i++] = 0x00;
    packet->m_body[i++] = 0x00;
    packet->m_body[i++] = 0x00;

    //版本
    packet->m_body[i++] = 0x01;
    //编码规格
    packet->m_body[i++] = sps[1];
    packet->m_body[i++] = sps[2];
    packet->m_body[i++] = sps[3];
    packet->m_body[i++] = 0xFF;

    //整个sps
    packet->m_body[i++] = 0xE1;
    //sps长度
    packet->m_body[i++] = (sps_len >> 8) & 0xff;
    packet->m_body[i++] = sps_len & 0xff;
    memcpy(&packet->m_body[i], sps, sps_len);
    i += sps_len;

    //pps
    packet->m_body[i++] = 0x01;
    packet->m_body[i++] = (pps_len >> 8) & 0xff;
    packet->m_body[i++] = (pps_len) & 0xff;
    memcpy(&packet->m_body[i], pps, pps_len);

    //视频
    packet->m_packetType = RTMP_PACKET_TYPE_VIDEO;
    packet->m_nBodySize = bodySize;
    //随意分配一个管道（尽量避开rtmp.c中使用的）
    packet->m_nChannel = 10;
    //sps pps没有时间戳
    packet->m_nTimeStamp = 0;
    //不使用绝对时间
    packet->m_hasAbsTimestamp = 0;
    packet->m_headerType = RTMP_PACKET_SIZE_MEDIUM;

    videoCallback(packet);
}

void VideoChannel::sendFrame(int type, uint8_t *payload, int i_payload) {
    if (payload[2] == 0x00) {
        i_payload -= 4;
        payload += 4;
    } else {
        i_payload -= 3;
        payload += 3;
    }
    int bodySize = 9 + i_payload;
    auto *packet = new RTMPPacket;
    RTMPPacket_Alloc(packet, bodySize);

    packet->m_body[0] = 0x27;
    if (type == NAL_SLICE_IDR) {
        packet->m_body[0] = 0x17;
        LOGE("关键帧");
    }
    //类型
    packet->m_body[1] = 0x01;
    //时间戳
    packet->m_body[2] = 0x00;
    packet->m_body[3] = 0x00;
    packet->m_body[4] = 0x00;
    //数据长度 int 4个字节
    packet->m_body[5] = (i_payload >> 24) & 0xff;
    packet->m_body[6] = (i_payload >> 16) & 0xff;
    packet->m_body[7] = (i_payload >> 8) & 0xff;
    packet->m_body[8] = (i_payload) & 0xff;

    //图片数据
    memcpy(&packet->m_body[9], payload, i_payload);

    packet->m_hasAbsTimestamp = 0;
    packet->m_nBodySize = bodySize;
    packet->m_packetType = RTMP_PACKET_TYPE_VIDEO;
    packet->m_nChannel = 0x10;
    packet->m_headerType = RTMP_PACKET_SIZE_LARGE;
    videoCallback(packet);
}

VideoChannel::~VideoChannel() {
    if (videoCodec) {
        x264_encoder_close(videoCodec);
        videoCodec = 0;
    }
    if (pic_in) {
        x264_picture_clean(pic_in);
        DELETE(pic_in);
    }
}
