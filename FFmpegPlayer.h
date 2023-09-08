#ifndef FFMPEGPLAYER_H
#define FFMPEGPLAYER_H

#include <memory>
#include <thread>
#include <atomic>
#include <functional>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

class FFmpegPlayer
{
public:
    explicit FFmpegPlayer();
    ~FFmpegPlayer();
public:
    std::atomic_bool m_started;
    void on_frame(void* frame);

public:
    void start_preview(const std::string &media_url);
    void set_preview_callback(std::function<void (uint8_t*/*data*/,int/*w*/,int/*h*/)> callback);
    void stop_preview();

private:
    void cleanup();
    void reconnect();

    AVFormatContext *m_formatCtx;
    AVCodecContext *m_codecCtx;
    AVDictionary *m_options;
    AVFrame *m_frame;
    AVPacket *m_packet;
    SwsContext *m_swsCtx;

    char local_media_url[128];
    std::unique_ptr<std::thread> p_bio_player_thread;

    std::function<void (uint8_t*/*data*/,int/*w*/,int/*h*/)> preview_callback;
    int m_videoStream;
};

#endif // FFMPEGPLAYER_H
