#include <FFmpegPlayer.h>

void console_log_callback(void *avcl, int level, const char *fmt, va_list vl)
{
    if (level > av_log_get_level())
        return;
    vfprintf(stderr, fmt, vl);
    fflush(stderr);
}

FFmpegPlayer::FFmpegPlayer()
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
//    m_started = false;
//    if (player_future.valid())
//        player_future.get();
    cleanup();
}

//    PLAYER_STATUS_IDLE->PLAYER_STATUS_BLOCK->PLAYER_STATUS_PENDING_STOP->PLAYER_STATUS_IDLE
int FFmpegPlayer::start_preview(const std::string &media_url)
{
    std::lock_guard<std::mutex> lk(player_lock);
    //PLAYER_STATUS_IDLE
    if(m_status.load(std::memory_order_acquire) == PLAYER_STATUS_IDLE)
    {
        std::future<int> futureResult = std::async(std::launch::async, [&, media_url](){
            on_start_preview(media_url);
            m_status.store(PLAYER_STATUS_BLOCK, std::memory_order_release);
            av_log(NULL, AV_LOG_ERROR, "stop1");
            return this->process_player_task(media_url);
        });
        //future observer
        std::thread([&, media_url,futureResult = std::move(futureResult), p_status = std::ref(m_status)]() mutable {
            int result = futureResult.get();
            av_log(NULL, AV_LOG_ERROR, "stop1");
            p_status.get().store(PLAYER_STATUS_IDLE, std::memory_order_release);
            //You can block on_stop_preview && You can restart in on_stop_preview
            on_stop_preview(media_url);
            av_log(NULL, AV_LOG_ERROR, "stop2");
        }).detach();
        return 0;
    }
    else
    {
        //PLAYER_STATUS_BLOCK/PLAYER_STATUS_PENDING_STOP
        return -1;
    }
}
int FFmpegPlayer::stop_preview()
{
    std::lock_guard<std::mutex> lk(player_lock);
    uint8_t expected = PLAYER_STATUS_BLOCK;
    //PLAYER_STATUS_BLOCK->PLAYER_STATUS_PENDING_STOP
    if (m_status.compare_exchange_strong(expected, PLAYER_STATUS_PENDING_STOP, std::memory_order_release, std::memory_order_relaxed))
        return 0;
    else
    {
        //PLAYER_STATUS_BLOCK/PLAYER_STATUS_IDLE
        return -1;
    }
}
int FFmpegPlayer::process_player_task(const std::string &media_url)
{
//    std::lock_guard<std::mutex> lk(player_lock);
//    bool expected = false;
//    if (!m_started.compare_exchange_strong(expected, true, std::memory_order_release, std::memory_order_relaxed))
//        return;
//    player_future = std::async(std::launch::async, [&, media_url]()
//                               {
                                    // Open input stream
    std::string local_media_url = media_url;
//    on_start_preview(local_media_url);
    m_formatCtx = avformat_alloc_context();
    if (!m_formatCtx)
    {
        av_log(NULL, AV_LOG_ERROR, "Failed to allocate AVFormatContext");
        cleanup();
        return -1;
    }
    if (int ret = avformat_open_input(&m_formatCtx, local_media_url.data(), nullptr, &m_options) < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "Failed to open input stream: %s\n", local_media_url.data());
        cleanup();
        return -1;
    }
    if (int ret = avformat_find_stream_info(m_formatCtx, nullptr) < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "Failed to find stream information: %s\n", local_media_url.data());
        cleanup();
        return -1;
    }

    // Find video stream
    for (unsigned int i = 0; i < m_formatCtx->nb_streams; i++)
    {
        if (m_formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            m_videoStream = i;
            av_log(NULL, AV_LOG_ERROR, "m_videoStream: %d\n", i);
        }
        else if (m_formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
        {
            m_audioStream = i;
            av_log(NULL, AV_LOG_ERROR, "m_audioStream: %d\n", i);
        }
        else
        {
            av_log(NULL, AV_LOG_ERROR, "unknown codec_type: %d\n", m_formatCtx->streams[i]->codecpar->codec_type);
        }
    }
    // Video
    if (m_videoStream == -1)
    {
        av_log(NULL, AV_LOG_ERROR, "Failed to find video stream: %s\n", local_media_url.data());
        cleanup();
        return -1;
    }
    else
    {
        // Get codec context
        m_codecCtx = avcodec_alloc_context3(nullptr);
        if (!m_codecCtx)
        {
            av_log(NULL, AV_LOG_ERROR, "Failed to allocate AVFormatContext");
            cleanup();
            return -1;
        }
        if (avcodec_parameters_to_context(m_codecCtx, m_formatCtx->streams[m_videoStream]->codecpar) < 0)
        {
            av_log(NULL, AV_LOG_ERROR, "Failed to get codec context: %s\n", local_media_url.data());
            cleanup();
            return -1;
        }
        const AVCodec *codec = avcodec_find_decoder(m_codecCtx->codec_id);
        if (!codec)
        {
            av_log(NULL, AV_LOG_ERROR, "Failed to find decoder: %s\n", local_media_url.data());
            cleanup();
            return -1;
        }
        if (avcodec_open2(m_codecCtx, codec, nullptr) < 0)
        {
            av_log(NULL, AV_LOG_ERROR, "Failed to find codec2: %s\n", local_media_url.data());
            cleanup();
            return -1;
        }
        // Allocate frame and packet
        m_frame = av_frame_alloc();
        if (!m_frame)
        {
            av_log(NULL, AV_LOG_ERROR, "Failed to allocate AVFrame");
            cleanup();
            return -1;
        }
        // Initialize scaler
        m_swsCtx = sws_getContext(m_codecCtx->width, m_codecCtx->height, m_codecCtx->pix_fmt,
                                  m_codecCtx->width, m_codecCtx->height, AV_PIX_FMT_RGBA,
                                  SWS_BILINEAR, nullptr, nullptr, nullptr);
        if (!m_swsCtx)
        {
            av_log(NULL, AV_LOG_ERROR, "Failed to initialize scaler");
            cleanup();
            return -1;
        }
    }
    // Audio
    if (m_audioStream == -1)
    {
        av_log(NULL, AV_LOG_WARNING, "Failed to find audio stream: %s\n", local_media_url.data());
    }
    else
    {
        m_audioCodecCtx = avcodec_alloc_context3(nullptr);
        if (!m_audioCodecCtx)
        {
            av_log(NULL, AV_LOG_ERROR, "Failed to allocate AVCodecContext for audio");
            cleanup();
            return -1;
        }
        if (avcodec_parameters_to_context(m_audioCodecCtx, m_formatCtx->streams[m_audioStream]->codecpar) < 0)
        {
            av_log(NULL, AV_LOG_ERROR, "Failed to get audio codec context: %s\n", local_media_url.data());
            cleanup();
            return -1;
        }
        const AVCodec *audioCodec = avcodec_find_decoder(m_audioCodecCtx->codec_id);
        if (!audioCodec)
        {
            av_log(NULL, AV_LOG_ERROR, "Failed to find audio decoder");
            cleanup();
            return -1;
        }

        if (avcodec_open2(m_audioCodecCtx, audioCodec, nullptr) < 0)
        {
            av_log(NULL, AV_LOG_ERROR, "Failed to open audio codec");
            cleanup();
            return -1;
        }
        m_audioFrame = av_frame_alloc();
        if (!m_audioFrame)
        {
            av_log(NULL, AV_LOG_ERROR, "Failed to allocate audio AVFrame");
            cleanup();
            return -1;
        }
        m_swrCtx = swr_alloc();
        if(!m_swrCtx)
        {
            av_log(NULL, AV_LOG_ERROR, "Failed to allocate m_swrCtx");
            cleanup();
            return -1;
        }
        AVChannelLayout outChannelLayout = AV_CHANNEL_LAYOUT_STEREO;
        outChannelLayout.nb_channels = 2;
        if (swr_alloc_set_opts2(&m_swrCtx, &outChannelLayout, AV_SAMPLE_FMT_S16, 48000,
                    &(m_audioCodecCtx->ch_layout), m_audioCodecCtx->sample_fmt, m_audioCodecCtx->sample_rate, 0, NULL) != 0)
        {
            av_log(NULL, AV_LOG_ERROR, "swr_alloc_set_opts2 fail.\n");
            cleanup();
            return -1;
        }
        if(int swr_init_ret = swr_init(m_swrCtx); swr_init_ret != 0)
        {
            av_log(NULL, AV_LOG_ERROR, "swr_init fail.\n");
            cleanup();
            return -1;
        }
        //swr_free
    }

    m_packet = av_packet_alloc();
    if (!m_packet)
    {
        av_log(NULL, AV_LOG_ERROR, "Failed to allocate AVPacket");
        cleanup();
        return -1;
    }

    while (av_read_frame(m_formatCtx, m_packet) >= 0)
    {
        //    PLAYER_STATUS_IDLE->PLAYER_STATUS_BLOCK->PLAYER_STATUS_PENDING_STOP->PLAYER_STATUS_IDLE
        if(m_status == PLAYER_STATUS_BLOCK)
        {
            if (m_packet->stream_index == m_videoStream)
            {
                if (avcodec_send_packet(m_codecCtx, m_packet) >= 0)
                {
                    while (avcodec_receive_frame(m_codecCtx, m_frame) >= 0)
                    {
                        // producer
                        if(frame_consumed.load(std::memory_order_acquire))
                        {
                            m_frame_cache.reset(new FrameCache());
                            if (!m_frame_cache->m_cache)
                            {
                                av_log(NULL, AV_LOG_ERROR, "Failed to allocate AVFrame");
                                av_packet_unref(m_packet);
                                cleanup();
                                return -1;
                            }
                            m_frame_cache->m_cache->width = m_codecCtx->width;
                            m_frame_cache->m_cache->height = m_codecCtx->height;
                            m_frame_cache->m_cache->format = AV_PIX_FMT_RGBA;
                            if (av_frame_get_buffer(m_frame_cache->m_cache, 0) < 0)
                            {
                                av_log(NULL, AV_LOG_ERROR, "Failed to allocate RGB buffer");
                                av_packet_unref(m_packet);
                                cleanup();
                                return -1;
                            }
                            sws_scale(m_swsCtx, m_frame->data, m_frame->linesize, 0, m_codecCtx->height,
                                      m_frame_cache->m_cache->data, m_frame_cache->m_cache->linesize);

                            frame_consumed.store(false, std::memory_order_release);
                            on_new_frame_avaliable();
                        }
                    }
                }
            }
            else if (m_packet->stream_index == m_audioStream)
            {
                // Audio decoding code
                if (avcodec_send_packet(m_audioCodecCtx, m_packet) >= 0)
                {
                    while (avcodec_receive_frame(m_audioCodecCtx, m_audioFrame) >= 0)
                    {
                        m_audio_frame_cache.reset(new FrameCache());
                        m_audio_frame_cache->m_cache->format = AV_SAMPLE_FMT_S16;
                        AVChannelLayout outChannelLayout = AV_CHANNEL_LAYOUT_STEREO;
                        outChannelLayout.nb_channels = 2;
                        m_audio_frame_cache->m_cache->ch_layout = outChannelLayout;
                        m_audio_frame_cache->m_cache->nb_samples = m_audioFrame->nb_samples;
                        if (av_frame_get_buffer(m_audio_frame_cache->m_cache, 0) < 0)
                        {
                            av_log(NULL, AV_LOG_ERROR, "Failed to allocate audio buffer");
                            av_packet_unref(m_packet);
                            cleanup();
                            return -1;
                        }
                        swr_convert(m_swrCtx, m_audio_frame_cache->m_cache->data, m_audio_frame_cache->m_cache->nb_samples,
                                   (const uint8_t **)m_audioFrame->data, m_audioFrame->nb_samples);
                        on_new_audio_frame_avaliable(m_audio_frame_cache);
                    }
                }
            }
            av_packet_unref(m_packet);
        }
        else if(m_status == PLAYER_STATUS_PENDING_STOP)
        {
            av_packet_unref(m_packet);
            cleanup();
            return 0;
        }
        else
        {
            av_log(NULL, AV_LOG_ERROR, "Unexpected Status!");
            av_packet_unref(m_packet);
            cleanup();
            return -1;
        }
    }
//    });
    return 0;
}



void FFmpegPlayer::on_start_preview(const std::string &media_url){};
void FFmpegPlayer::on_new_frame_avaliable(){};
void FFmpegPlayer::on_new_audio_frame_avaliable(std::shared_ptr<FrameCache> m_frame_cache){};
void FFmpegPlayer::on_stop_preview(const std::string &media_url){};
void FFmpegPlayer::on_ffmpeg_error(){};

void FFmpegPlayer::cleanup()
{
    // Free resources
    if (m_swsCtx)
    {
        sws_freeContext(m_swsCtx);
        m_swsCtx = nullptr;
    }
    if (m_swrCtx)
    {
        swr_free(&m_swrCtx);
        m_swrCtx = nullptr;
    }
    if (m_frame)
    {
        av_frame_free(&m_frame);
        m_frame = nullptr;
    }
    if (m_audioFrame)
    {
        av_frame_free(&m_audioFrame);
        m_audioFrame = nullptr;
    }
    if (m_codecCtx)
    {
        avcodec_free_context(&m_codecCtx);
        m_codecCtx = nullptr;
    }
    if (m_audioCodecCtx)
    {
        avcodec_free_context(&m_audioCodecCtx);
        m_audioCodecCtx = nullptr;
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
