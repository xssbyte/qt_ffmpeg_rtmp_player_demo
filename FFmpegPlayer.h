#ifndef FFMPEGPLAYER_H
#define FFMPEGPLAYER_H

#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include <future>
#include <functional>
#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h> //视频采样转换
#include <libswresample/swresample.h> //音频采样转换
#include <libavutil/imgutils.h> //图像操作
}
/**
* @brief         FFmpegPlayer class
* FFmpeg 播放器类,单生产者单消费者模型中的生产者
* 子类播放widget需要继承FFmpegPlayer并重写回调。
* 如果只是取帧，不需要显示或者自己实现显示，也可以直接实例化
* @author        Samuel<13321133339@163.com>
* @date          2023/09/23
*/
class FFmpegPlayer
{
public:
    FFmpegPlayer();
    ~FFmpegPlayer();
public:
    /**
     * @brief start_player 等于start_preview,但是可以和其他选项一起启动,只是简单判断其他选项是否正确设置上了
     * 关闭调用stop_preview(),stop_preview()会按顺序停止所有功能
     * 比如我要启动预览的同时,启动录制.就可以这样调用:start_player(url, start_local_record(file));
     * @param media_url 等于start_preview
     * @param option_ret 其他选项的返回值的和
     * @return 0成功
     */
    int start_player(const std::string &media_url, int option_ret = 0);
    /**
     * @brief start_preview 异步的开始播放任务，启动ffmpeg解析url播放，成功会回调on_start_preview
     * @param media_url 媒体url
     * @return 0成功
     */
    int start_preview(const std::string &media_url);
    /**
     * @brief stop_preview 异步的停止预览，清理ffmpeg资源，异步停止播放线程，成功停止会回调on_stop_preview.也会停止其他功能
     * @return 0成功
     */
    int stop_preview();
    /**
     * @brief start_local_record 异步的开始录制视频帧，rtsp转mp4保存，预览时可以录像，非预览时不能录像，成功会回调on_start_record
     * @param output_file 输出文件
     * @return 0成功
     */
    int start_record(const std::string &output_file);
    /**
     * @brief stop_local_record 异步的停止录像，成功会回调on_stop_record
     * @return 0成功
     */
    int stop_record();
    /**
     * @brief capture_frame 取帧
     * @param output_file 输出文件
     * @return 0成功
     */
    int capture_frame(const std::string &output_file);
//TODO:errno
//    enum FFmpegPlayerErrno
//    {
//        STATUS_OK = 0,
//        INVALID_STATUS = -1,
//        NO_MEMORY = -2,
//    };
protected:
    //TODO:优化这里
    //正常应该复用AVFrame,调用av_frame_unref()将frame复位
    //这里是为了把帧取走而这样设计的,会导致频繁申请释放堆内存
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

    //播放线程错误回调
    virtual void on_player_error(int errnum);
    //播放线程启动成功回调
    virtual void on_preview_start(const std::string& media_url, const int width, const int height);
    //播放线程关闭回调,不管是否报错,播放线程结束就回调,可以在里面重启播放器,但是如果流不可用就会一直循环
    virtual void on_preview_stop(const std::string& media_url);
    //新的video帧可用时的回调,用于消费帧的数据,帧使用原子量同步,同步消费或者存入缓冲区
    virtual void on_new_frame_avaliable(std::shared_ptr<FrameCache> m_frame_cache);
    //新的audio帧可用时的回调,用于消费帧的数据,同步消费或者存入缓冲区
    virtual void on_new_audio_frame_avaliable(std::shared_ptr<FrameCache> m_frame_cache);
    //开始录像的回调
    virtual void on_recorder_start(const std::string& file);
    //停止录像的回调
    virtual void on_recorder_stop(const std::string& file);

    //单生产者单消费者模型，可以使用原子量同步并避免memcpy
    std::atomic_bool frame_consumed;
    //使用智能指针,内部只维护最新生产的一帧的指针引用计数,外部可以取走智能指针做缓存
    std::shared_ptr<FrameCache> m_frame_cache;
    std::shared_ptr<FrameCache> m_audio_frame_cache;
private:

    enum PlayerStatus : uint8_t
    {
        PLAYER_STATUS_IDLE,
        PLAYER_STATUS_PENDING_START,
        PLAYER_STATUS_PLAYING,
        PLAYER_STATUS_PENDING_STOP
    };
    enum RecorderStatus : uint8_t
    {
        RECORDER_STATUS_IDLE,
        RECORDER_STATUS_PENDING_START,
        RECORDER_STATUS_RECORDING,
        RECORDER_STATUS_PENDING_STOP
    };
    enum CaptureFrameStatus : uint8_t
    {
        CAPTURE_FRAME_OFF,
        CAPTURE_FRAME_ON,
    };

    void on_stream_avaliable();
    void on_packet_received();
    void on_stream_unavaliable();

    void preprocess_incoming_frame(AVFrame* frame);

    //player
    int player_process(const std::string &media_url);
    void cleanup();

    //recorder
    std::string recorder_file_path;
    int recorder_begin(const std::string& file);
    int recorder_end();
    void cleanup_recorder();
    //capture frame
    std::string capture_frame_file_path;
    int save_frame_as_jpeg(AVFrame* frame, const std::string& file);

    //IO interrupt callback
    static int stream_interrupt_callback(void* ptr);

    //player
    AVFormatContext *m_formatCtx = nullptr;
    AVCodecContext *m_codecCtx = nullptr;
    AVCodecContext *m_audioCodecCtx = nullptr;
    AVDictionary *m_options = nullptr;
    AVFrame *m_frame = nullptr;
    AVFrame *m_audioFrame = nullptr;
    AVPacket *m_packet = nullptr;
    SwsContext *m_swsCtx = nullptr;
    SwrContext *m_swrCtx = nullptr;
    AVCodecParameters *m_videoCodecParam = nullptr;
    int m_videoStream = -1;
    int m_audioStream = -1;

    //recorder
    AVFormatContext *m_outStreamContext = nullptr;
    AVStream *m_outStream = nullptr;

    //status
    std::mutex player_mutex;
    std::mutex recorder_mutex;
    PlayerStatus player_status = PLAYER_STATUS_IDLE;
    RecorderStatus recorder_status = RECORDER_STATUS_IDLE;
    CaptureFrameStatus capture_frame_status = CAPTURE_FRAME_OFF;

    int viewport_width = 0;
    int viewport_height = 0;

    std::unique_ptr<std::future<void>> m_playerFutureObserver = nullptr;
};

#endif // FFMPEGPLAYER_H
