#ifndef IAUDIOBACKEND_H
#define IAUDIOBACKEND_H

namespace BugAV {

class AudioParams;

class IAudioBackEnd {
public:
    virtual ~IAudioBackEnd() = default;

    virtual int open(int wanted_channel_layout,
                      int wanted_nb_channels,
                      int wanted_sample_rate,
                      AudioParams *audio_hw_params) = 0;

    virtual int close() = 0;



};
}
#endif // IAudioBaIAUDIOBACKEND_H
