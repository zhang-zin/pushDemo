
#ifndef PUSHDEMO_AUDIOCHANNEL_H
#define PUSHDEMO_AUDIOCHANNEL_H

#include "librtmp/rtmp.h"
#include <faac.h>
#include <unistd.h>
#include <cstring>
#include "macro.h"

class AudioChannel {
    typedef void (*AudioCallback)(RTMPPacket *packet);

public:
    void setAudioCallback(AudioCallback audioCallback);

    void setAudioEncInfo(int sampleRateInHz, int channels);

    int getInputSamples();

    void encodeData(int8_t *data);

    RTMPPacket * getAudioTag();

    ~AudioChannel();

private:
    AudioCallback audioCallback;
    faacEncHandle audioCodec;
    int mChannels;
    u_long inputSamples;
    u_long maxOutputBytes;
    u_char *buffer = 0;
};

#endif //PUSHDEMO_AUDIOCHANNEL_H
