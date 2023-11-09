#include <QPlayerWidget.h>
#include <QDateTime> // for log

QPlayerWidget::QPlayerWidget(QWidget *parent) : QWidget(parent),
    m_audioPlayer(new QAudioPlayer(8192 * 16))
{
    QPalette palette = this->palette();
    palette.setColor(QPalette::Background, Qt::black);
    setAutoFillBackground(true);
    setPalette(palette);
}
void QPlayerWidget::paintEvent(QPaintEvent *event)
{
    if(!FFmpegPlayer::frame_consumed.load(std::memory_order_acquire))
    {
        QPainter painter(this);
        QImage frame_image(m_frame_cache->m_cache->data[0],
                m_frame_cache->m_cache->width,
                m_frame_cache->m_cache->height,
                QImage::Format_RGBA8888);
        //这里拷贝了一帧,根据需求确定使用移动构造还是拷贝构造
        //如果只需要播放,不需要录像的情况下是单生产者单消费者,可以直接std::move移走帧数据
        //录像/取帧+播放情况下是单生产者多消费者,必须拷贝,否则录制/取帧的帧为空
        m_image = frame_image.copy();
        //m_image = std::move(frame_image);
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
void QPlayerWidget::start_local_record(const std::string &output_file)
{
    FFmpegPlayer::start_record(output_file);
}
void QPlayerWidget::stop_local_record()
{
    FFmpegPlayer::stop_record();
}

void QPlayerWidget::on_new_frame_avaliable(std::shared_ptr<FrameCache> m_frame_cache)
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
