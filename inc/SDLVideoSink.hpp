#ifndef SDL_VIDEO_SINK_HPP
#define SDL_VIDEO_SINK_HPP

#include <liveMedia.hh>

#ifdef __cplusplus
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavdevice/avdevice.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/pixdesc.h>
#include <libavfilter/avfiltergraph.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_video.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_mutex.h>
#include <SDL2/SDL.h>
}
#endif


class SDLVideoSink : public MediaSink {
public:

    constexpr static unsigned RECEIVE_BUFFER_SIZE = 100 * 1000;

    static SDLVideoSink *createNew(UsageEnvironment &env, MediaSubsession &subsession, char const *streamId = nullptr);

private:
    SDLVideoSink(UsageEnvironment &env, MediaSubsession &subsession, char const *streamId);

    ~SDLVideoSink() override;

    static void afterGettingFrame(void *clientData, unsigned frameSize,
                                  unsigned numTruncatedBytes,
                                  struct timeval presentationTime,
                                  unsigned durationInMicroseconds);

    void afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes, timeval presentationTime);

private:

    u_int8_t *fReceiveBuffer;
    MediaSubsession &fSubsession;
    char *fStreamId;

    // ffmpeg
    AVPacket* packet = nullptr;
    AVFrame* frame = nullptr;
    AVCodecContext* codecContext = nullptr;

    // SDL
    struct PlayContext {
        SDL_Renderer *renderer;
        SDL_Texture *texture;
        SDL_mutex *mutex;
        SDL_Window *window;
        SDL_Event event;
    } playContext;

    Boolean continuePlaying() override;

    void initFFmpeg();
};


#endif //SDL_VIDEO_SINK_HPP
