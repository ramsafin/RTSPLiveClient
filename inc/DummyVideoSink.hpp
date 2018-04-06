#ifndef LIVERTSPCLIENT_DUMMYVIDEOSINK_HPP
#define LIVERTSPCLIENT_DUMMYVIDEOSINK_HPP

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
}
#endif

#define DUMMY_SINK_RECEIVE_BUFFER_SIZE 30000

class DummySink : public MediaSink {
public:

    static DummySink *createNew(UsageEnvironment &env, MediaSubsession &subsession, char const *streamId = nullptr);

private:
    DummySink(UsageEnvironment &env, MediaSubsession &subsession, char const *streamId);

    ~DummySink() override;

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

    Boolean continuePlaying() override;

    void initFFmpeg();
};


#endif //LIVERTSPCLIENT_DUMMYVIDEOSINK_HPP
