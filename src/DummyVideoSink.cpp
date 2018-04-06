#include "DummyVideoSink.hpp"
#include <cassert>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

DummySink *DummySink::createNew(UsageEnvironment &env, MediaSubsession &subsession, char const *streamId) {
    return new DummySink(env, subsession, streamId);
}

DummySink::DummySink(UsageEnvironment &env, MediaSubsession &subsession, char const *streamId)
        : MediaSink(env), fSubsession(subsession) {
    fStreamId = strDup(streamId);
    fReceiveBuffer = new u_int8_t[DUMMY_SINK_RECEIVE_BUFFER_SIZE];
    memset(fReceiveBuffer, 0, DUMMY_SINK_RECEIVE_BUFFER_SIZE);
    initFFmpeg();
}

DummySink::~DummySink() {
    delete[] fReceiveBuffer;
    delete[] fStreamId;

    avcodec_close(codecContext);
    av_frame_free(&frame);
    av_packet_free(&packet);
}

void DummySink::afterGettingFrame(void *clientData, unsigned frameSize, unsigned numTruncatedBytes,
                                  timeval presentationTime, unsigned durationInMicroseconds) {
    auto sink = (DummySink *) clientData;
    sink->afterGettingFrame(frameSize, numTruncatedBytes, presentationTime);
}

void DummySink::afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes, timeval presentationTime) {

    // fixme: add start code to fReceiveBuffer
    uint8_t start_code[4] = {0x0, 0x0, 0x0, 0x1};
    packet->size = frameSize + 4;
    uint8_t buf[frameSize + 4];
    memcpy(buf, start_code, 4);
    memcpy(buf + 4, fReceiveBuffer, frameSize);
    packet->data = buf;

    if (avcodec_send_packet(codecContext, packet) != 0) {
        envir() << "Error while send packet to decoder" << "\n";
    }

    if (avcodec_receive_frame(codecContext, frame) == 0) {

        // todo
        cv::Mat rgb;
        cv::Mat raw(frame->height, frame->width, CV_8UC2, frame->data[0]);
        cv::cvtColor(raw, rgb, CV_YUV2BGR);

        cv::imshow("IMG", rgb);

        if (cv::waitKey(1) != -1) {
            exit(0);
        }
    }

    // Then continue, to request the next frame of data:
    continuePlaying();
}

Boolean DummySink::continuePlaying() {

    if (fSource == nullptr) return False; // sanity check (should not happen)

    // Request the next frame of data from our input source.  "afterGettingFrame()" will get called later, when it arrives:
    fSource->getNextFrame(fReceiveBuffer, DUMMY_SINK_RECEIVE_BUFFER_SIZE,
                          afterGettingFrame, this,
                          onSourceClosure, this);
    return True;
}

void DummySink::initFFmpeg() {

    av_log_set_level(AV_LOG_WARNING);

    av_register_all();
    avcodec_register_all();

    auto codec  = avcodec_find_decoder(AV_CODEC_ID_HEVC);
    assert(codec);

    codecContext = avcodec_alloc_context3(codec);
    assert(codecContext);

    avcodec_open2(codecContext, codec, nullptr);

    packet = av_packet_alloc();
    av_init_packet(packet);

    frame = av_frame_alloc();
}
