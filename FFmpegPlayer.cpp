#include <FFmpegPlayer.h>

void console_log_callback(void *avcl, int level, const char *fmt, va_list vl) {
    if (level > av_log_get_level()) return;
    vfprintf(stderr, fmt, vl);
    fflush(stderr);
}

FFmpegPlayer::FFmpegPlayer()
    : m_formatCtx(nullptr), m_codecCtx(nullptr), m_started(false), m_options(nullptr), m_frame(nullptr), m_packet(nullptr), m_swsCtx(nullptr), m_videoStream(-1)
{
    avformat_network_init();
    av_log_set_level(AV_LOG_INFO);
    av_log_set_callback(console_log_callback);

    av_dict_set(&m_options, "reconnect", "1", 0);
    av_dict_set(&m_options, "fflags", "nobuffer", 0);
    av_dict_set(&m_options, "reconnect_streamed", "1", 0);
    av_dict_set(&m_options, "reconnect_delay_max", "100", 0);
}

FFmpegPlayer::~FFmpegPlayer()
{
    m_started = false;
    if (player_future.valid())
        player_future.get();
    cleanup();
}
void FFmpegPlayer::set_preview_callback(std::function<void(uint8_t * /*data*/, int /*w*/, int /*h*/)> callback)
{
    std::lock_guard<std::mutex> lk(player_lock);
    preview_callback = std::move(callback);
}
void FFmpegPlayer::start_preview(const std::string &media_url)
{
    std::lock_guard<std::mutex> lk(player_lock);
    bool expected = false;
    if(!m_started.compare_exchange_strong(expected, true, std::memory_order_release, std::memory_order_relaxed))
        return;
    player_future = std::async(std::launch::async,
                               std::bind([&, media_url](std::function<void(uint8_t * /*data*/, int /*w*/, int /*h*/)> &preview_callback_internal) {
                                    // Open input stream
    std::string local_media_url = media_url;
    on_start_preview(local_media_url);

    m_formatCtx = avformat_alloc_context();
    if (!m_formatCtx)
    {
        av_log(NULL, AV_LOG_ERROR, "Failed to allocate AVFormatContext");
        cleanup();
        return;
    }
    if (int ret = avformat_open_input(&m_formatCtx, local_media_url.data(), nullptr, &m_options) < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "Failed to open input stream: %s\n", local_media_url.data());
        cleanup();
        return;
    }
    if (int ret = avformat_find_stream_info(m_formatCtx, nullptr) < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "Failed to find stream information: %s\n", local_media_url.data());
        cleanup();
        return;
    }

    // Find video stream
    for (unsigned int i = 0; i < m_formatCtx->nb_streams; i++)
    {
        if (m_formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            m_videoStream = i;
            break;
        }
    }
    if (m_videoStream == -1)
    {
        av_log(NULL, AV_LOG_ERROR, "Failed to find video stream: %s\n", local_media_url.data());
        cleanup();
        return;
    }

    // Get codec context
    m_codecCtx = avcodec_alloc_context3(nullptr);
    if (!m_codecCtx)
    {
        av_log(NULL, AV_LOG_ERROR, "Failed to allocate AVFormatContext");
        cleanup();
        return;
    }
    if (avcodec_parameters_to_context(m_codecCtx, m_formatCtx->streams[m_videoStream]->codecpar) < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "Failed to get codec context: %s\n", local_media_url.data());
        cleanup();
        return;
    }
    const AVCodec *codec = avcodec_find_decoder(m_codecCtx->codec_id);
    if (!codec)
    {
        av_log(NULL, AV_LOG_ERROR, "Failed to find decoder: %s\n", local_media_url.data());
        cleanup();
        return;
    }
    if (avcodec_open2(m_codecCtx, codec, nullptr) < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "Failed to find codec2: %s\n", local_media_url.data());
        cleanup();
        return;
    }

    // Allocate frame and packet
    m_frame = av_frame_alloc();
    if (!m_frame)
    {
        av_log(NULL, AV_LOG_ERROR, "Failed to allocate AVFrame");
        cleanup();
        return;
    }
    m_packet = av_packet_alloc();
    if (!m_packet)
    {
        av_log(NULL, AV_LOG_ERROR, "Failed to allocate AVPacket");
        cleanup();
        return;
    }

    // Initialize scaler
    m_swsCtx = sws_getContext(m_codecCtx->width, m_codecCtx->height, m_codecCtx->pix_fmt,
                              m_codecCtx->width, m_codecCtx->height, AV_PIX_FMT_RGBA,
                              SWS_BILINEAR, nullptr, nullptr, nullptr);
    if (!m_swsCtx)
    {
        av_log(NULL, AV_LOG_ERROR, "Failed to initialize scaler");
        cleanup();
        return;
    }
    while (av_read_frame(m_formatCtx, m_packet) >= 0)
    {
        if(m_started)
        {
            if (m_packet->stream_index == m_videoStream)
            {
                if (avcodec_send_packet(m_codecCtx, m_packet) >= 0)
                {
                    while (avcodec_receive_frame(m_codecCtx, m_frame) >= 0)
                    {
                        // Convert to RGBA
                        AVFrame *rgbFrame = av_frame_alloc();
                        if (!rgbFrame)
                        {
                            av_log(NULL, AV_LOG_ERROR, "Failed to allocate AVFrame");
                            av_packet_unref(m_packet);
                            cleanup();
                            return;
                        }
                        rgbFrame->width = m_codecCtx->width;
                        rgbFrame->height = m_codecCtx->height;
                        rgbFrame->format = AV_PIX_FMT_RGBA;
                        if (av_frame_get_buffer(rgbFrame, 0) < 0)
                        {
                            av_log(NULL, AV_LOG_ERROR, "Failed to allocate RGB buffer");
                            av_frame_free(&rgbFrame);
                            av_packet_unref(m_packet);
                            cleanup();
                            return;
                        }
                        sws_scale(m_swsCtx, m_frame->data, m_frame->linesize, 0, m_codecCtx->height,
                                  rgbFrame->data, rgbFrame->linesize);
                        // Callback
                        on_new_frame_avaliable(rgbFrame->data[0], m_codecCtx->width, m_codecCtx->height);

                        av_frame_free(&rgbFrame);
                    }
                }
            }
            av_packet_unref(m_packet);
        }
        else
        {
            av_packet_unref(m_packet);
            on_stop_preview(local_media_url);
            return;
        }
    }
                               },
                                         std::ref(preview_callback)));
}

void FFmpegPlayer::stop_preview()
{
    std::lock_guard<std::mutex> lk(player_lock);
    bool expected = true;
    if(!m_started.compare_exchange_strong(expected, false, std::memory_order_release, std::memory_order_relaxed))
        return;
    if (player_future.valid())
        player_future.get();
    cleanup();
}

void FFmpegPlayer::on_new_frame_avaliable(uint8_t* data,int w,int h)
{
    if(preview_callback)
        preview_callback(data, w, h);
}
void FFmpegPlayer::on_start_preview(const std::string& url){};
void FFmpegPlayer::on_stop_preview(const std::string& url){};
void FFmpegPlayer::on_ffmpeg_error(){};

void FFmpegPlayer::cleanup()
{
    // Free resources
    if (m_swsCtx)
    {
        sws_freeContext(m_swsCtx);
        m_swsCtx = nullptr;
    }
    if (m_frame)
    {
        av_frame_free(&m_frame);
        m_frame = nullptr;
    }
    if (m_codecCtx)
    {
        avcodec_free_context(&m_codecCtx);
        m_codecCtx = nullptr;
    }
    if (m_formatCtx)
    {
        avformat_close_input(&m_formatCtx);
        avformat_free_context(m_formatCtx);
        m_formatCtx = nullptr;
    }
    if (m_packet)
    {
        av_packet_free(&m_packet);
        m_packet = nullptr;
    }
    if (m_options)
    {
        av_dict_free(&m_options);
        m_options = nullptr;
    }
}
