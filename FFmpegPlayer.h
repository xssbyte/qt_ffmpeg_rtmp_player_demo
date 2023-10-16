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
#include <libswresample/swresample.h>
}
/**
* @brief         FFmpegPlayer class
* FFmpeg 播放器类，单生产者单消费者
* 子类播放widget需要继承FFmpegPlayer并重写回调。
* 如果只是取帧，不需要显示或者自己实现显示，我们也可以直接实例化
* @author        Samuel<13321133339@163.com>
* @date          2023/09/23
*/
class FFmpegPlayer
{
public:
    explicit FFmpegPlayer();
    ~FFmpegPlayer();
public:

public:
    /**
     * @brief start_preview 异步的开始播放任务，启动ffmpeg解析url播放，成功会回调on_start_preview
     * @param media_url 媒体url
     * @return 0成功
     */
    int start_preview(const std::string &media_url);
    /**
     * @brief stop_preview 异步的停止预览，清理ffmpeg资源，异步停止播放线程，成功停止会回调on_stop_preview
     * @return 0成功
     */
    int stop_preview();
    /**
     * @brief start_local_record 异步的开始录像，rtsp转mp4保存，预览时可以录像，非预览时不能录像，成功会回调on_start_record
     * @return 0成功
     */
    int start_local_record(const std::string &output_file);
    /**
     * @brief stop_local_record 异步的停止录像，成功会回调on_stop_record
     * @return 0成功
     */
    int stop_local_record();

//    enum FFmpegPlayerErrno
//    {
//        STATUS_OK = 0,
//        INVALID_STATUS = -1,
//        NO_MEMORY = -2,
//    };
protected:
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
    //播放线程启动成功回调
    virtual void on_start_preview(const std::string& media_url);
    //新的video帧可用回调，这里不传入帧的数据，帧使用原子量同步。也可以传帧数据，同步消费或者存入缓冲区
    virtual void on_new_frame_avaliable();
    //新的audio帧可用回调，传入帧的数据,同步消费或者存入缓冲区
    virtual void on_new_audio_frame_avaliable(std::shared_ptr<FrameCache> m_frame_cache);
    //播放线程关闭回调，不管是否报错，播放线程结束就回调
    virtual void on_stop_preview(const std::string& media_url);
    //播放线程错误回调
    virtual void on_ffmpeg_error(int errnum);
    //开始录像的回调
    virtual void on_start_record(const std::string& file);
    //停止录像的回调
    virtual void on_stop_record(const std::string& file);

    //额外feature可用的回调，当流初始化完成，这个回调被调用。
    void on_param_initialized();

    //单生产者单消费者模型，可以使用原子量同步并避免memcpy
    std::atomic_bool frame_consumed = true;
    std::unique_ptr<FrameCache> m_frame_cache;
    std::shared_ptr<FrameCache> m_audio_frame_cache;
private:

    //播放器和录像的状态机实现
    enum PlayerStatus : uint8_t
    {
        PLAYER_STATUS_IDLE,

        PLAYER_STATUS_BLOCK,
        PLAYER_STATUS_PENDING_STOP
    };
    enum RecorderStatus : uint8_t
    {
        RECORDER_STATUS_IDLE,
        RECORDER_STATUS_RECORDING,
        RECORDER_STATUS_PENDING_STOP
    };
    template <typename T, typename = std::enable_if_t<std::is_enum_v<T> && std::is_same_v<std::underlying_type_t<T>, uint8_t>>>
    class StateMachine {
    public:
        StateMachine(T status) : currentState(status) {}
        bool transition(uint8_t from, uint8_t to) {
            return currentState.compare_exchange_strong(from, to, std::memory_order_release, std::memory_order_relaxed);
        }
        uint8_t state() const {
            return currentState.load(std::memory_order_acquire);
        }
    private:
        std::atomic_uint8_t currentState;
    };

    int process_player_task(const std::string &media_url);
    int process_recorder_task(const std::string& file);
    void cleanup();
    void cleanup_recorder();

    AVFormatContext *m_formatCtx = nullptr;
    AVCodecContext *m_codecCtx = nullptr;
    AVCodecContext *m_audioCodecCtx = nullptr;
    AVDictionary *m_options = nullptr;
    AVFrame *m_frame = nullptr;
    AVFrame *m_audioFrame = nullptr;
    AVPacket *m_packet = nullptr;
    SwsContext *m_swsCtx = nullptr;
    SwrContext *m_swrCtx = nullptr;

    //for record
    AVFormatContext *m_outStreamContext = nullptr;
    AVCodecParameters *m_codecParam = nullptr;
    AVStream *m_outStream = nullptr;

    std::mutex player_lock;

    StateMachine<PlayerStatus> m_player_status = PLAYER_STATUS_IDLE;
    StateMachine<RecorderStatus> m_recorder_status = RECORDER_STATUS_IDLE;
    int m_videoStream = -1;
    int m_audioStream = -1;
};

#endif // FFMPEGPLAYER_H
