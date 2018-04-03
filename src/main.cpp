#include <cassert>
#include <algorithm>
#include <iostream>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

#ifdef __cplusplus
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavfilter/avfiltergraph.h>
#include <libavfilter/buffersink.h>
}
#endif

// forward
int decode(AVCodecContext *codecContext, AVFrame *frame, AVPacket *packet);

int main() {

    av_log_set_level(AV_LOG_VERBOSE);

    av_register_all();
    avcodec_register_all();
    avformat_network_init();

    auto formatCtx = avformat_alloc_context();
    AVDictionary *opts = nullptr;
//    av_dict_set(&opts, "rtsp_transport", "udp", 0);

    int statCode = avformat_open_input(&formatCtx, "rtsp://localhost:8554/camera", nullptr, &opts);
    assert(statCode >= 0);

    statCode = avformat_find_stream_info(formatCtx, nullptr);
    assert(statCode >= 0);

    av_dump_format(formatCtx, 0, "camera", 0);

    AVCodec *codec = nullptr;
    int videoStreamIdx = av_find_best_stream(formatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, &codec, 0);
    assert(videoStreamIdx >= 0);
    assert(codec);

    AVStream *videoStream = formatCtx->streams[videoStreamIdx];

    AVCodecContext *codecContext = avcodec_alloc_context3(codec);
    assert(codecContext);

    statCode = avcodec_parameters_to_context(codecContext, videoStream->codecpar);
    assert(statCode >= 0);

    statCode = avcodec_open2(codecContext, codec, nullptr);

    AVPacket *packet = av_packet_alloc();
    av_init_packet(packet);

    AVFrame *encodedFrame = av_frame_alloc();

    AVFrame *bgrFrame = av_frame_alloc();
    bgrFrame->width = videoStream->codecpar->width;
    bgrFrame->height = videoStream->codecpar->height;
    bgrFrame->format = AV_PIX_FMT_BGR24;
    av_frame_get_buffer(bgrFrame, 0);

    auto converter = sws_getCachedContext(nullptr,
                                          bgrFrame->width, bgrFrame->height, AV_PIX_FMT_YUV420P,
                                          bgrFrame->width, bgrFrame->height, AV_PIX_FMT_BGR24,
                                          SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);

    cv::namedWindow("Video Stream");

    // decoding

    while (av_read_frame(formatCtx, packet) == 0) {

        if (packet->stream_index == videoStreamIdx) {

            if (decode(codecContext, encodedFrame, packet)) {

                sws_scale(converter, reinterpret_cast<const uint8_t *const *>(encodedFrame->data),
                          encodedFrame->linesize, 0, bgrFrame->height, bgrFrame->data, bgrFrame->linesize);

                cv::Mat bgr(bgrFrame->height, bgrFrame->width, CV_8UC3, bgrFrame->data[0]);

                cv::imshow("Video Stream", bgr);

                if (cv::waitKey(1) != -1) {
                    break;
                }
            }
        }

    }

}


int decode(AVCodecContext *codecContext, AVFrame *frame, AVPacket *packet) {

    int statCode = avcodec_send_packet(codecContext, packet);

    if (statCode < 0) {
        std::cerr << "Error sending a packet for decoding: " << std::endl;
        return statCode;
    }

    statCode = avcodec_receive_frame(codecContext, frame);

    if (statCode == AVERROR(EAGAIN) || statCode == AVERROR_EOF) {
        std::cerr << "No frames available or end of file has been reached" << std::endl;
        return statCode;
    }

    if (statCode < 0) {
        std::cerr << "Error during decoding" << std::endl;
        return statCode;
    }

    return true;
}