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
    av_log(NULL, AV_LOG_ERROR, "[FFMPEG_VERION_INFO]%s\n", av_version_info());

    av_dict_set(&m_options, "tune", "zerolatency", 0);
    av_dict_set(&m_options, "fflags", "nobuffer", 0);
    av_dict_set(&m_options, "reconnect", "0", 0);
    av_dict_set(&m_options, "reconnect_streamed", "0", 0);
    av_dict_set(&m_options, "reconnect_delay_max", "5", 0);
}

FFmpegPlayer::~FFmpegPlayer()
{
    player_status = PLAYER_STATUS_PENDING_STOP;
}
int FFmpegPlayer::start_player(const std::string &media_url, int option_ret)
{
    if(option_ret != 0)
        return option_ret;
    return start_preview(media_url);
}
int FFmpegPlayer::start_preview(const std::string &media_url)
{
    std::lock_guard<std::mutex> pl(player_mutex);
    if(player_status == PLAYER_STATUS_IDLE)
    {
        //wait until last on_preview_stop finish
        if(m_playerFutureObserver != nullptr)
        {
            m_playerFutureObserver->get();
        }
        //future observer
        m_playerFutureObserver = std::make_unique<std::future<void>>(std::async(std::launch::async, [&, media_url]() mutable {
            //future processor
            std::future<int> futureResult = std::async(std::launch::async, [&, media_url](){
                player_status = PLAYER_STATUS_PENDING_START;
                if(int ret = process_player_task(media_url) < 0)
                {
                    return ret;
                }
                return 0;
            });
            int ret = futureResult.get();
            if(ret < 0)
            {
                on_player_error(ret);
            }
            //player thread not running
            player_status = PLAYER_STATUS_IDLE;
            //You can block on_stop_preview && You can restart preview in on_stop_preview
            on_preview_stop(media_url);
        }));
        return 0;
    }
    else
    {
        return -1;
    }
}
int FFmpegPlayer::stop_preview()
{
    std::lock_guard<std::mutex> pl(player_mutex);
    if(player_status == PLAYER_STATUS_PLAYING)
    {
        player_status = PLAYER_STATUS_PENDING_STOP;
        return 0;
    }
    return -1;
}
int FFmpegPlayer::start_local_record(const std::string &output_file)
{
    std::lock_guard<std::mutex> pl(player_mutex);
    std::lock_guard<std::mutex> rl(recorder_mutex);
    if((player_status == PLAYER_STATUS_IDLE) && (recorder_status == RECORDER_STATUS_IDLE))
    {
        recorder_status = RECORDER_STATUS_PENDING_START;
        recorder_file_path = output_file;
        return 0;
    }
    if((player_status == PLAYER_STATUS_PLAYING) && (recorder_status == RECORDER_STATUS_IDLE))
    {
        if(int ret = recorder_begin(output_file) < 0)
        {
            return ret;
        }
        recorder_status = RECORDER_STATUS_RECORDING;
        std::future<void> futureResult = std::async(std::launch::async, [&, output_file](){
            on_recorder_start(output_file);
            return;
        });
        return 0;
    }
    return -1;
}
int FFmpegPlayer::stop_local_record()
{
    std::lock_guard<std::mutex> pl(player_mutex);
    std::lock_guard<std::mutex> r1(recorder_mutex);
    if((player_status == PLAYER_STATUS_PLAYING) && (recorder_status == RECORDER_STATUS_RECORDING))
    {
        recorder_status = RECORDER_STATUS_PENDING_STOP;
        return 0;
    }
    if((player_status == PLAYER_STATUS_IDLE) && (recorder_status == RECORDER_STATUS_PENDING_START))
    {
        recorder_status = RECORDER_STATUS_IDLE;
        recorder_file_path.clear();
        return 0;
    }
    return -1;
}
int FFmpegPlayer::recorder_begin(const std::string& file)
{
    recorder_file_path = file;
    m_outStreamContext = avformat_alloc_context();
    if (int ret = avformat_alloc_output_context2(&m_outStreamContext, NULL, NULL, recorder_file_path.c_str()) < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "Failed to allocate AVFormatContext");
        cleanup_recorder();
        return ret;
    }
    if (int ret = avio_open(&m_outStreamContext->pb, recorder_file_path.c_str(), AVIO_FLAG_WRITE) < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "Failed to open output file");
        cleanup_recorder();
        return ret;
    }
    m_outStream = avformat_new_stream(m_outStreamContext, NULL);
    if (!m_outStream)
    {
        av_log(NULL, AV_LOG_ERROR, "Failed to allocate AVStream");
        cleanup_recorder();
        return -1;
    }
    if(int ret = avcodec_parameters_copy(m_outStream->codecpar, m_videoCodecParam) < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "Failed to copy avcodec param");
        cleanup_recorder();
        return ret;
    }
    if (int ret = avformat_write_header(m_outStreamContext, NULL) < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "Failed to write output header");
        cleanup_recorder();
        return ret;
    }
    av_log(NULL, AV_LOG_DEBUG, "start recorder");
    return 0;
}
int FFmpegPlayer::recorder_end()
{
    if (int ret = av_write_trailer(m_outStreamContext) < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "Failed to write output trailer");
        cleanup_recorder();
        return ret;
    }
    cleanup_recorder();
    return 0;
}

int FFmpegPlayer::process_player_task(const std::string &media_url)
{
    std::string local_media_url = media_url;
    m_formatCtx = avformat_alloc_context();
    if (!m_formatCtx)
    {
        av_log(NULL, AV_LOG_ERROR, "Failed to allocate AVFormatContext");
        cleanup();
        return -1;
    }
    std::chrono::time_point<std::chrono::high_resolution_clock> last_time = std::chrono::high_resolution_clock::now();
    m_formatCtx->interrupt_callback.opaque = &last_time;
    m_formatCtx->interrupt_callback.callback = this->stream_interrupt_callback;
    //would block
    if (int ret = avformat_open_input(&m_formatCtx, local_media_url.data(), nullptr, &m_options) < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "Failed to open input stream: %s\n", local_media_url.data());
        cleanup();
        return ret;
    }
    if (int ret = avformat_find_stream_info(m_formatCtx, nullptr) < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "Failed to find stream information: %s\n", local_media_url.data());
        cleanup();
        return ret;
    }
    // Find video stream
    for (unsigned int i = 0; i < m_formatCtx->nb_streams; i++)
    {
        if (m_formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            m_videoStream = i;
            m_videoCodecParam = m_formatCtx->streams[i]->codecpar;
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
        if (int ret = avcodec_parameters_to_context(m_codecCtx, m_formatCtx->streams[m_videoStream]->codecpar) < 0)
        {
            av_log(NULL, AV_LOG_ERROR, "Failed to get codec context: %s\n", local_media_url.data());
            cleanup();
            return ret;
        }
        const AVCodec *codec = avcodec_find_decoder(m_codecCtx->codec_id);
        if (!codec)
        {
            av_log(NULL, AV_LOG_ERROR, "Failed to find decoder: %s\n", local_media_url.data());
            cleanup();
            return -1;
        }
        if (int ret = avcodec_open2(m_codecCtx, codec, nullptr) < 0)
        {
            av_log(NULL, AV_LOG_ERROR, "Failed to find codec2: %s\n", local_media_url.data());
            cleanup();
            return ret;
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
        if (int ret = avcodec_parameters_to_context(m_audioCodecCtx, m_formatCtx->streams[m_audioStream]->codecpar) < 0)
        {
            av_log(NULL, AV_LOG_ERROR, "Failed to get audio codec context: %s\n", local_media_url.data());
            cleanup();
            return ret;
        }
        const AVCodec *audioCodec = avcodec_find_decoder(m_audioCodecCtx->codec_id);
        if (!audioCodec)
        {
            av_log(NULL, AV_LOG_ERROR, "Failed to find audio decoder");
            cleanup();
            return -1;
        }

        if (int ret = avcodec_open2(m_audioCodecCtx, audioCodec, nullptr) < 0)
        {
            av_log(NULL, AV_LOG_ERROR, "Failed to open audio codec");
            cleanup();
            return ret;
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
//        AVChannelLayout outChannelLayout = AV_CHANNEL_LAYOUT_STEREO;
        AVChannelLayout outChannelLayout;
        outChannelLayout.order = AV_CHANNEL_ORDER_NATIVE;
        outChannelLayout.u.mask = AV_CH_LAYOUT_STEREO;
        outChannelLayout.nb_channels = 2;
        if (swr_alloc_set_opts2(&m_swrCtx, &outChannelLayout, AV_SAMPLE_FMT_S16, 48000,
                    &(m_audioCodecCtx->ch_layout), m_audioCodecCtx->sample_fmt, m_audioCodecCtx->sample_rate, 0, NULL) != 0)
        {
            av_log(NULL, AV_LOG_ERROR, "swr_alloc_set_opts2 fail.\n");
            cleanup();
            return -1;
        }
        if(int ret = swr_init(m_swrCtx) < 0)
        {
            av_log(NULL, AV_LOG_ERROR, "swr_init fail.\n");
            cleanup();
            return ret;
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
    //initialization finish
    on_stream_avaliable();
    on_preview_start(media_url, width, height);

    last_time = std::chrono::high_resolution_clock::now();
    while (av_read_frame(m_formatCtx, m_packet) >= 0)
    {
        if(player_status == PLAYER_STATUS_PLAYING)
        {
            //video stream
            if (m_packet->stream_index == m_videoStream)
            {
                if (avcodec_send_packet(m_codecCtx, m_packet) >= 0)
                {
                    on_packet_received();
                    while (avcodec_receive_frame(m_codecCtx, m_frame) >= 0)
                    {
                        // producer
                        if(frame_consumed.load(std::memory_order_acquire) == true)
                        {
                            m_frame_cache.reset(new FrameCache());
                            if (!m_frame_cache->m_cache)
                            {
                                av_log(NULL, AV_LOG_ERROR, "Failed to allocate AVFrame");
                                av_packet_unref(m_packet);
                                cleanup();
                                on_stream_unavaliable();
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
                                on_stream_unavaliable();
                                return -1;
                            }
                            sws_scale(m_swsCtx, m_frame->data, m_frame->linesize, 0, m_codecCtx->height,
                                      m_frame_cache->m_cache->data, m_frame_cache->m_cache->linesize);

                            on_new_frame_avaliable();
                            frame_consumed.store(false, std::memory_order_release);
                        }
                    }
                }
            }
            //audio stream
            else if (m_packet->stream_index == m_audioStream)
            {
                // Audio decoding code
                if (avcodec_send_packet(m_audioCodecCtx, m_packet) >= 0)
                {
                    while (avcodec_receive_frame(m_audioCodecCtx, m_audioFrame) >= 0)
                    {
                        m_audio_frame_cache.reset(new FrameCache());
                        m_audio_frame_cache->m_cache->format = AV_SAMPLE_FMT_S16;
//                        AVChannelLayout outChannelLayout = AV_CHANNEL_LAYOUT_STEREO;
                        AVChannelLayout outChannelLayout;
                        outChannelLayout.order = AV_CHANNEL_ORDER_NATIVE;
                        outChannelLayout.u.mask = AV_CH_LAYOUT_STEREO;
                        outChannelLayout.nb_channels = 2;
                        m_audio_frame_cache->m_cache->ch_layout = outChannelLayout;
                        m_audio_frame_cache->m_cache->nb_samples = m_audioFrame->nb_samples;
                        if (av_frame_get_buffer(m_audio_frame_cache->m_cache, 0) < 0)
                        {
                            av_log(NULL, AV_LOG_ERROR, "Failed to allocate audio buffer");
                            av_packet_unref(m_packet);
                            cleanup();
                            on_stream_unavaliable();
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
        else if(player_status == PLAYER_STATUS_PENDING_STOP)
        {
            av_packet_unref(m_packet);
            cleanup();
            on_stream_unavaliable();
            return 0;
        }
        else
        {
            av_log(NULL, AV_LOG_ERROR, "Unexpected Status! %d", player_status);
            av_packet_unref(m_packet);
            cleanup();
            on_stream_unavaliable();
            return -1;
        }
        last_time = std::chrono::high_resolution_clock::now();
    }
    //EOF or error
    av_log(NULL, AV_LOG_INFO, "EOF\n");
    av_packet_unref(m_packet);
    cleanup();
    on_stream_unavaliable();
    return 0;
}
int FFmpegPlayer::stream_interrupt_callback(void* opaque)
{
    auto current_time = std::chrono::high_resolution_clock::now();
    auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - *(std::chrono::time_point<std::chrono::high_resolution_clock>*)opaque);
    if (elapsed_time.count() > 5000) {
        return 1;
    }
    return 0;
}


void FFmpegPlayer::on_new_frame_avaliable(){};
void FFmpegPlayer::on_new_audio_frame_avaliable(std::shared_ptr<FrameCache> m_frame_cache){};
void FFmpegPlayer::on_preview_start(const std::string& media_url, const int width, const int height){
    av_log(NULL, AV_LOG_DEBUG, "[%s] %s\n", __FUNCTION__, media_url.c_str());
};
void FFmpegPlayer::on_preview_stop(const std::string &media_url){
    av_log(NULL, AV_LOG_DEBUG, "[%s] %s\n", __FUNCTION__, media_url.c_str());
};
void FFmpegPlayer::on_player_error(int errnum){
    av_log(NULL, AV_LOG_ERROR, "[%s] %d\n", __FUNCTION__, errnum);
};
void FFmpegPlayer::on_recorder_start(const std::string& file){
    av_log(NULL, AV_LOG_DEBUG, "[%s] %s\n", __FUNCTION__, file.c_str());
};
void FFmpegPlayer::on_recorder_stop(const std::string& file){
    av_log(NULL, AV_LOG_DEBUG, "[%s] %s\n", __FUNCTION__, file.c_str());
};

void FFmpegPlayer::on_stream_avaliable()
{
    std::lock_guard<std::mutex> pl(player_mutex);
    std::lock_guard<std::mutex> rl(recorder_mutex);
    if(player_status == PLAYER_STATUS_PENDING_START)
    {
        player_status = PLAYER_STATUS_PLAYING;
        width = m_codecCtx->width;
        height = m_codecCtx->height;
    }
}
void FFmpegPlayer::on_packet_received()
{
    std::lock_guard<std::mutex> rl(recorder_mutex);
    if(recorder_status == RECORDER_STATUS_RECORDING)
    {
        if(int ret = av_interleaved_write_frame(m_outStreamContext, m_packet) < 0)
        {
            on_player_error(ret);
        }
    }
    if(recorder_status == RECORDER_STATUS_PENDING_START)
    {
        if(int ret = recorder_begin(recorder_file_path) < 0)
        {
            recorder_status = RECORDER_STATUS_IDLE;
            on_player_error(ret);
        }
        else
        {
            recorder_status = RECORDER_STATUS_RECORDING;
            on_recorder_start(recorder_file_path);
            //init sync
            if(int ret = av_interleaved_write_frame(m_outStreamContext, m_packet) < 0)
            {
                on_player_error(ret);
            }
        }
    }
    else if(recorder_status == RECORDER_STATUS_PENDING_STOP)
    {
        if(int ret = recorder_end() < 0)
        {
            recorder_status = RECORDER_STATUS_IDLE;
            on_player_error(ret);
        }
        else
        {
            recorder_status = RECORDER_STATUS_IDLE;
            on_recorder_stop(recorder_file_path);
        }
    }
}
void FFmpegPlayer::on_stream_unavaliable()
{
    std::lock_guard<std::mutex> pl(player_mutex);
    std::lock_guard<std::mutex> rl(recorder_mutex);
    player_status = PLAYER_STATUS_PENDING_STOP;
    if(recorder_status == RECORDER_STATUS_RECORDING)
    {
        if(int ret = recorder_end() < 0)
        {
            recorder_status = RECORDER_STATUS_IDLE;
            on_player_error(ret);
        }
        else
        {
            recorder_status = RECORDER_STATUS_IDLE;
            on_recorder_stop(recorder_file_path);
        }
    }
}
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
    if (m_videoCodecParam)
    {
        m_videoCodecParam = nullptr;
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
    m_videoStream = -1;
    m_audioStream = -1;
    width = 0;
    height = 0;
}
void FFmpegPlayer::cleanup_recorder()
{
    if(m_outStreamContext)
    {
        avio_close(m_outStreamContext->pb);
        avformat_free_context(m_outStreamContext);
        m_outStreamContext = nullptr;
    }
    if(m_outStream)
    {
        m_outStream = nullptr;
    }
}
