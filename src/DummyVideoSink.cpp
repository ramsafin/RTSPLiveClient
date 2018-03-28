#include "DummyVideoSink.hpp"
#include <H265VideoStreamFramer.hh>
#include <assert.h>

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

DummySink *DummySink::createNew(UsageEnvironment &env, MediaSubsession &subsession, char const *streamId) {
    return new DummySink(env, subsession, streamId);
}

DummySink::DummySink(UsageEnvironment &env, MediaSubsession &subsession, char const *streamId)
        : MediaSink(env), fSubsession(subsession) {
    fStreamId = strDup(streamId);
    fReceiveBuffer = new u_int8_t[DUMMY_SINK_RECEIVE_BUFFER_SIZE];
    initFFmpeg();
}

DummySink::~DummySink() {
    delete[] fReceiveBuffer;
    delete[] fStreamId;
}

void DummySink::afterGettingFrame(void *clientData, unsigned frameSize, unsigned numTruncatedBytes,
                                  timeval presentationTime, unsigned durationInMicroseconds) {
    auto sink = (DummySink *) clientData;
    sink->afterGettingFrame(frameSize, numTruncatedBytes, presentationTime);
}

void DummySink::afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes, timeval presentationTime) {

    /*
    if (fStreamId != nullptr) envir() << "Stream \"" << fStreamId << "\"; ";

    envir() << fSubsession.mediumName() << "/" << fSubsession.codecName() << ":\tReceived " << frameSize << " bytes";

    if (numTruncatedBytes > 0) envir() << " (with " << numTruncatedBytes << " bytes truncated)";

    char uSecsStr[6 + 1]; // used to output the 'microseconds' part of the presentation time

    sprintf(uSecsStr, "%06u", (unsigned) presentationTime.tv_usec);

    envir() << ".\tPresentation time: " << (int) presentationTime.tv_sec << "." << uSecsStr;

    if (fSubsession.rtpSource() != nullptr && !fSubsession.rtpSource()->hasBeenSynchronizedUsingRTCP()) {
        envir() << "!"; // mark the debugging output to indicate that this presentation time is not RTCP-synchronized
    }

    envir() << "\tNPT: " << fSubsession.getNormalPlayTime(presentationTime) << "\n";

     */


    // Then continue, to request the next frame of data:
    continuePlaying();
}

Boolean DummySink::continuePlaying() {

    if (fSource == nullptr) return False; // sanity check (should not happen)

    // Request the next frame of data from our input source.  "afterGettingFrame()" will get called later, when it arrives:
    fSource->getNextFrame(reinterpret_cast<unsigned char *>(fReceiveBuffer), DUMMY_SINK_RECEIVE_BUFFER_SIZE,
                          afterGettingFrame, this,
                          onSourceClosure, this);
    return True;
}

void DummySink::initFFmpeg() {

    av_log_set_level(AV_LOG_DEBUG);

    av_register_all();
    avcodec_register_all();
    avformat_network_init();

    auto formatCtx = avformat_alloc_context();

    auto inputFormat = av_find_input_format("rtsp");

    int statCode = avformat_open_input(&formatCtx, "rtsp://localhost:8554/camera", inputFormat, nullptr);
    assert(statCode >= 0);


}
