#include "DummyVideoSink.hpp"
#include <cassert>

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
    ((DummySink *) clientData)->afterGettingFrame(frameSize, numTruncatedBytes, presentationTime);
}

void DummySink::afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes, timeval presentationTime) {

    int startCodeLength = 4;
    uint8_t start_code[4] = {0x0, 0x0, 0x0, 0x1};

    packet->size = frameSize + startCodeLength;
    packet->data = new uint8_t[frameSize + 4];

    memcpy(packet->data, start_code, 4);
    memcpy(packet->data + 4, fReceiveBuffer, frameSize);

    if (avcodec_send_packet(codecContext, packet) != 0) {
        envir() << "Error while send packet to decoder"  << "\n";
    }

    while (avcodec_receive_frame(codecContext, frame) == 0) {

        SDL_LockMutex(playContext.mutex);

        SDL_UpdateYUVTexture(playContext.texture, nullptr, frame->data[0], frame->linesize[0],
                             frame->data[1], frame->linesize[1], frame->data[2], frame->linesize[2]);

        SDL_RenderClear(playContext.renderer);
        SDL_RenderCopy(playContext.renderer, playContext.texture, nullptr, nullptr);
        SDL_RenderPresent(playContext.renderer);

        SDL_UnlockMutex(playContext.mutex);
    }

    // fixme: check if memory leak occurs
    av_packet_unref(packet);

    SDL_PollEvent(&playContext.event);

    switch (playContext.event.type) {
        case SDL_QUIT:
            SDL_Quit();
            exit(0);
        default:
            break;
    }

    // Then continue, to request the next frame of data
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

    auto codec = avcodec_find_decoder(AV_CODEC_ID_HEVC);
    assert(codec);

    codecContext = avcodec_alloc_context3(codec);
    assert(codecContext);

    avcodec_open2(codecContext, codec, nullptr);

    packet = av_packet_alloc();
    av_init_packet(packet);

    frame = av_frame_alloc();

    // SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER)) {
        exit(-1);
    }

    playContext = {nullptr};

    playContext.window = SDL_CreateWindow("SDL YUV Player", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, 0);

    playContext.renderer = SDL_CreateRenderer(playContext.window, -1, 0);
    playContext.texture = SDL_CreateTexture(playContext.renderer, SDL_PIXELFORMAT_YV12, SDL_TEXTUREACCESS_STREAMING,
                                            640, 480);
    playContext.mutex = SDL_CreateMutex();
}
