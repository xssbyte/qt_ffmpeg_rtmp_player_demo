#include <rtmpPlayer.h>
#include <QtConcurrent/QtConcurrent>

RTMPPlayer::RTMPPlayer(QWidget *widget, QObject *parent)
    : QObject(parent), m_widget(widget), m_formatCtx(nullptr), m_codecCtx(nullptr),
      m_frame(nullptr), m_packet(nullptr), m_timer(nullptr), m_swsCtx(nullptr), m_options(nullptr),
      m_videoStream(-1), m_started(false)
{
    qDebug() << __FUNCTION__ << QThread::currentThreadId();
    avformat_network_init();
    av_log_set_level(AV_LOG_ERROR);
}

RTMPPlayer::~RTMPPlayer()
{
    cleanup();
}
void RTMPPlayer::restart()
{
    qDebug() << __FUNCTION__ << QThread::currentThreadId();;
    m_timer->singleShot(2000,this,[&](){
        stop();
        start(m_url);
    });
}
void RTMPPlayer::start(const QString &url)
{
    qDebug() << __FUNCTION__ << QThread::currentThreadId();
    if (m_started) {
        return;
    }
    m_started = true;
    m_url = url;

    // Open input stream
    m_formatCtx = avformat_alloc_context();
    if (!m_formatCtx) {
        emit error("Failed to allocate AVFormatContext");
        return;
    }
    av_dict_set(&m_options, "reconnect", "1", 0);
    av_dict_set(&m_options, "reconnect_streamed", "1", 0);
    av_dict_set(&m_options, "reconnect_delay_max", "100", 0);
    if (avformat_open_input(&m_formatCtx, url.toUtf8().constData(), nullptr, &m_options) < 0) {
        emit error(QString("Failed to open input stream: %1").arg(url));
        cleanup();
        return;
    }
    if (avformat_find_stream_info(m_formatCtx, nullptr) < 0) {
        emit error(QString("Failed to find stream information: %1").arg(url));
        cleanup();
        return;
    }

    // Find video stream
    for (unsigned int i = 0; i < m_formatCtx->nb_streams; i++) {
        if (m_formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            m_videoStream = i;
            break;
        }
    }
    if (m_videoStream == -1) {
        emit error(QString("Failed to find video stream: %1").arg(url));
        cleanup();
        return;
    }

    // Get codec context
    m_codecCtx = avcodec_alloc_context3(nullptr);
    if (!m_codecCtx) {
        emit error("Failed to allocate AVCodecContext");
        cleanup();
        return;
    }
    if (avcodec_parameters_to_context(m_codecCtx, m_formatCtx->streams[m_videoStream]->codecpar) < 0) {
        emit error(QString("Failed to get codec context: %1").arg(url));
        cleanup();
        return;
    }
    const AVCodec *codec = avcodec_find_decoder(m_codecCtx->codec_id);
    if (!codec) {
        emit error(QString("Failed to find codec: %1").arg(url));
        cleanup();
        return;
    }
    if (avcodec_open2(m_codecCtx, codec, nullptr) < 0) {
        emit error(QString("Failed to open codec: %1").arg(url));
        cleanup();
        return;
    }

    // Allocate frame and packet
    m_frame = av_frame_alloc();
    if (!m_frame) {
        emit error("Failed to allocate AVFrame");
        cleanup();
        return;
    }
    m_packet = av_packet_alloc();
    if (!m_packet) {
        emit error("Failed to allocate AVPacket");
        cleanup();
        return;
    }

    // Initialize scaler
    m_swsCtx = sws_getContext(m_codecCtx->width, m_codecCtx->height, m_codecCtx->pix_fmt,
                              m_codecCtx->width, m_codecCtx->height, AV_PIX_FMT_YUVJ420P,
                              SWS_BILINEAR, nullptr, nullptr, nullptr);
    if (!m_swsCtx) {
        emit error("Failed to initialize scaler");
        cleanup();
        return;
    }

    // Create timer
    m_timer.reset(new QTimer());
    connect(m_timer.get(), &QTimer::timeout, this, &RTMPPlayer::updateFrame);
    connect(this, &RTMPPlayer::error, this, &RTMPPlayer::restart);
    // Start timer
    m_timer->start(16);
}

void RTMPPlayer::stop()
{
    qDebug() << __FUNCTION__ << QThread::currentThreadId();;
    if (!m_started) {
        return;
    }
    m_started = false;

    // Stop timer
    m_timer->stop();

    cleanup();
}

void RTMPPlayer::updateFrame()
{
    if (!m_started) {
        return;
    }
    // Seek to the latest frame
    if (m_formatCtx->pb && m_formatCtx->pb->eof_reached) {
        int64_t seek_target = m_formatCtx->streams[m_videoStream]->duration - AV_TIME_BASE / 10;
        int ret = av_seek_frame(m_formatCtx, m_videoStream, seek_target, AVSEEK_FLAG_BACKWARD);
        if (ret < 0) {
            qDebug() << "Failed to seek frame";
        }
    }
    // Decode frame
    if (av_read_frame(m_formatCtx, m_packet) >= 0) {
        if (m_packet->stream_index == m_videoStream) {
            if (avcodec_send_packet(m_codecCtx, m_packet) >= 0) {
                while (avcodec_receive_frame(m_codecCtx, m_frame) >= 0) {
                    // Scale frame
                    sws_scale(m_swsCtx, m_frame->data, m_frame->linesize, 0, m_codecCtx->height,
                              m_frame->data, m_frame->linesize);

                    // Convert to RGBA
                    AVFrame *rgbFrame = av_frame_alloc();
                    if (!rgbFrame) {
                        emit error("Failed to allocate AVFrame");
                        av_packet_unref(m_packet);
                        return;
                    }
                    rgbFrame->width = m_codecCtx->width;
                    rgbFrame->height = m_codecCtx->height;
                    rgbFrame->format = AV_PIX_FMT_RGBA;
                    if (av_frame_get_buffer(rgbFrame, 0) < 0) {
                        emit error("Failed to allocate RGB buffer");
                        av_frame_free(&rgbFrame);
                        av_packet_unref(m_packet);
                        return;
                    }
                    SwsContext *rgbSwsCtx = sws_getContext(m_codecCtx->width, m_codecCtx->height, m_codecCtx->pix_fmt,
                                                          m_codecCtx->width, m_codecCtx->height, AV_PIX_FMT_RGBA,
                                                          SWS_BILINEAR, nullptr, nullptr, nullptr);
                    if (!rgbSwsCtx) {
                        emit error("Failed to initialize scaler");
                        av_frame_free(&rgbFrame);
                        av_packet_unref(m_packet);
                        return;
                    }
                    sws_scale(rgbSwsCtx, m_frame->data, m_frame->linesize, 0, m_codecCtx->height,
                              rgbFrame->data, rgbFrame->linesize);
                    sws_freeContext(rgbSwsCtx);

                    // Create image
                    QImage image(rgbFrame->data[0], m_codecCtx->width, m_codecCtx->height,
                                 QImage::Format_RGBA8888);

                    // Deep copy
                    m_image = image.copy();
                    // Update widget
                    emit sigUpdateUI();

//                    m_widget->update();

                    av_frame_free(&rgbFrame);
                }
            }
        }
        av_packet_unref(m_packet);
    }
    else
    {
        stop();
        restart();
    }
}

void RTMPPlayer::cleanup()
{
    // Free resources
    if (m_swsCtx) {
        sws_freeContext(m_swsCtx);
        m_swsCtx = nullptr;
    }
    if (m_packet) {
        av_packet_free(&m_packet);
        m_packet = nullptr;
    }
    if (m_frame) {
        av_frame_free(&m_frame);
        m_frame = nullptr;
    }
    if (m_codecCtx) {
        avcodec_free_context(&m_codecCtx);
        m_codecCtx = nullptr;
    }
    if (m_formatCtx) {
        avformat_close_input(&m_formatCtx);
        avformat_free_context(m_formatCtx);
        m_formatCtx = nullptr;
    }
    if (m_options) {
        av_dict_free(&m_options);
        m_options = nullptr;
    }
}
void RTMPPlayer::getImage(QImage& image)
{
    image = m_image;
}
