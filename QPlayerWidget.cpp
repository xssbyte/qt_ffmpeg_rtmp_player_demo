#include "QPlayerWidget.h"

QPlayerWidget::QPlayerWidget(QWidget *parent) : QWidget(parent)
{
    // Set size and background color
    qDebug() << "FFmpeg version: " << av_version_info();
    QPalette palette = this->palette();
    palette.setColor(QPalette::Background, Qt::black);
    setAutoFillBackground(true);
    setPalette(palette);

    // 配置音频设备参数
    QAudioFormat format;
    format.setSampleRate(48000); // 采样率
    format.setChannelCount(2);  // 声道数
    format.setSampleSize(16);    // 采样位数
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::UnSignedInt);

    // 创建QAudioOutput对象
    audioOutput.reset(new QAudioOutput(format));
    audioOutput->setVolume(100);
    m_ringBuffer.open(QIODevice::ReadWrite | QIODevice::Unbuffered);
    audioOutput->start(&m_ringBuffer);
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
}
void QPlayerWidget::stop_preview()
{
    FFmpegPlayer::stop_preview();
}
void QPlayerWidget::on_new_frame_avaliable()
{
    this->update();
}
void QPlayerWidget::on_new_audio_frame_avaliable(std::shared_ptr<FrameCache> m_frame_cache)
{
    int bytes_written = 0;
    if(m_frame_cache->m_cache->linesize[0] != 0)
    {
        bytes_written = m_ringBuffer.write(reinterpret_cast<const char*>(m_frame_cache->m_cache->data[0]), m_frame_cache->m_cache->linesize[0]);
    }
    qDebug() << __FUNCTION__ << bytes_written << m_frame_cache->m_cache->linesize[0];
}
