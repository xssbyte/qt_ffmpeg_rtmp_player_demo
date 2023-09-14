#ifndef FFMPEGPLAYER_H
#define FFMPEGPLAYER_H

#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include <future>
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

public:
    /**
     * @brief start_preview 开始预览，启动ffmpeg解析url
     * @param media_url 媒体url
     */
    void start_preview(const std::string &media_url);
    /**
     * @brief set_preview_callback 设置预览时的回调，需要在start_preview之前调用
     * @param callback c++风格的回调函数，每收到一帧，callback都会被调用一次
     */
    void set_preview_callback(std::function<void (uint8_t*/*data*/,int/*w*/,int/*h*/)> callback);
    /**
     * @brief stop_preview 停止预览，清理ffmpeg资源
     */
    void stop_preview();

private:
    void cleanup();

    AVFormatContext *m_formatCtx;
    AVCodecContext *m_codecCtx;
    AVDictionary *m_options;
    AVFrame *m_frame;
    AVPacket *m_packet;
    SwsContext *m_swsCtx;

    std::future<void> player_future;
    std::mutex player_lock;
    std::function<void (uint8_t*/*data*/,int/*w*/,int/*h*/)> preview_callback;
    int m_videoStream;
};

#endif // FFMPEGPLAYER_H
