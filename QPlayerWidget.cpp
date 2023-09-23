#include <QPlayerWidget.h>

QPlayerWidget::QPlayerWidget(QWidget *parent) : QWidget(parent),
    m_audioPlayer(new QAudioPlayer(8192 * 16))
{
    // Set size and background color
    qDebug() << "FFmpeg version: " << av_version_info();
    QPalette palette = this->palette();
    palette.setColor(QPalette::Background, Qt::black);
    setAutoFillBackground(true);
    setPalette(palette);
}
void QPlayerWidget::paintEvent(QPaintEvent *event)
{
    qDebug() << __FUNCTION__;
    if(!FFmpegPlayer::frame_consumed.load(std::memory_order_acquire))
    {
        QPainter painter(this);
        QImage frame_image(m_frame_cache->m_cache->data[0],
                m_frame_cache->m_cache->width,
                m_frame_cache->m_cache->height,
                QImage::Format_RGBA8888);
        m_image = frame_image.copy();
        painter.drawImage(rect(), m_image);
        FFmpegPlayer::frame_consumed.store(true, std::memory_order_release);
    }
}
void QPlayerWidget::start_preview(const std::string &media_url)
{
    FFmpegPlayer::start_preview(media_url);
    m_audioPlayer->start_consume_audio();
}
void QPlayerWidget::stop_preview()
{
    FFmpegPlayer::stop_preview();
    m_audioPlayer->stop_consume_audio();
}
void QPlayerWidget::on_new_frame_avaliable()
{
    //update自动合并多余的重绘
    this->update();
}
void QPlayerWidget::on_new_audio_frame_avaliable(std::shared_ptr<FrameCache> m_frame_cache)
{
    if(m_audioPlayer)
    {
        m_audioPlayer->write(reinterpret_cast<const char*>(m_frame_cache->m_cache->data[0]), m_frame_cache->m_cache->linesize[0]);
    }
//    qDebug() << __FUNCTION__ << bytes_written << m_frame_cache->m_cache->linesize[0];
}
