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
/**
 * @brief FFmpeg 播放器类，单生产者单消费者
 * 子类需要继承FFmpegPlayer并重写回调
 */
class FFmpegPlayer
{
public:
    explicit FFmpegPlayer();
    ~FFmpegPlayer();
public:
    std::atomic_bool m_started;

public:
    /**
     * @brief start_preview 开始异步任务，启动ffmpeg解析url
     * @param media_url 媒体url
     */
    void start_preview(const std::string &media_url);
    /**
     * @brief stop_preview 停止预览，清理ffmpeg资源，同步等待播放线程结束
     */
    void stop_preview();


protected:
    //播放线程启动成功回调
    virtual void on_start_preview(const std::string& media_url);
    //新的帧可用回调，不传入帧的数据，帧使用原子量同步
    virtual void on_new_frame_avaliable();
    //播放线程关闭成功回调
    virtual void on_stop_preview(const std::string& media_url);
    //播放线程错误回调
    virtual void on_ffmpeg_error();

    class FrameCache
    {
    public:
        FrameCache()
        {
            m_cache = av_frame_alloc();
        };
        ~FrameCache()
        {
            av_frame_free(&m_cache);
        }
        AVFrame *m_cache;
    };
    //单生产者单消费者模型，可以使用原子量同步并避免memcpy
    std::atomic_bool frame_consumed = true;
    std::unique_ptr<FrameCache> m_frame_cache;

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
    int m_videoStream;
    int m_audioStream;
};

#endif // FFMPEGPLAYER_H
