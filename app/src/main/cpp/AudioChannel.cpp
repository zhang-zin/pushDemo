#include "AudioChannel.h"

void AudioChannel::setAudioCallback(AudioCallback audioCallback_) {
    this->audioCallback = audioCallback_;
}

/**
 * faac初始化
 * @param sampleRateInHz
 * @param channels
 */
void AudioChannel::setAudioEncInfo(int sampleRateInHz, int channels) {
    mChannels = channels;
    // 最新缓冲区大小
    audioCodec = faacEncOpen(sampleRateInHz, channels, &inputSamples, &maxOutputBytes);
    // 设置参数
    faacEncConfigurationPtr config = faacEncGetCurrentConfiguration(audioCodec);
    config->mpegVersion = MPEG4;
    config->aacObjectType = LOW;
    config->inputFormat = FAAC_INPUT_16BIT;
    // 编码出原始数据 既不是adts也不是adif
    config->outputFormat = 0;
    faacEncSetConfiguration(audioCodec, config);
    buffer = new u_char[maxOutputBytes];
}

int AudioChannel::getInputSamples() {
    return inputSamples;
}

void AudioChannel::encodeData(int8_t *data) {
    int byteLen = faacEncEncode(audioCodec, reinterpret_cast<int32_t *>(data), inputSamples, buffer,
                                maxOutputBytes);
    if (byteLen > 0) {
        int bodySize = 2 + byteLen;
        RTMPPacket *packet = new RTMPPacket;
        RTMPPacket_Alloc(packet, bodySize);
        packet->m_body[0] = 0XAF;
        if (mChannels == 1) {
            packet->m_body[0] = 0XAE;
        }
        packet->m_body[1] = 0x01;
        memcpy(&packet->m_body[2], buffer, byteLen);

        packet->m_hasAbsTimestamp = 0;
        packet->m_nBodySize = bodySize;
        packet->m_packetType = RTMP_PACKET_TYPE_AUDIO;
        packet->m_nChannel = 0x11;
        packet->m_headerType = RTMP_PACKET_SIZE_LARGE;
        audioCallback(packet);
    }
}

RTMPPacket *AudioChannel::getAudioTag() {
    u_char *buf;
    u_long len;
    //     编码器信息     解码
    int i = faacEncGetDecoderSpecificInfo(audioCodec, &buf, &len);
    LOGE("faacEncGetDecoderSpecificInfo: %d", i);
    int bodySize = 2 + len;
    RTMPPacket *packet = new RTMPPacket;
    RTMPPacket_Alloc(packet, bodySize);
    //双声道
    packet->m_body[0] = 0xAF;
    if (mChannels == 1) {
        packet->m_body[0] = 0xAE;
    }
    packet->m_body[1] = 0x00;
    //图片数据
    memcpy(&packet->m_body[2], buf, len);

    packet->m_hasAbsTimestamp = 0;
    packet->m_nBodySize = bodySize;
    packet->m_packetType = RTMP_PACKET_TYPE_AUDIO;
    packet->m_nChannel = 0x11;
    packet->m_headerType = RTMP_PACKET_SIZE_LARGE;
    return packet;
}

AudioChannel::~AudioChannel() {
    DELETE(buffer)
    if (audioCodec) {
        faacEncClose(audioCodec);
        audioCodec = nullptr;
    }
}

