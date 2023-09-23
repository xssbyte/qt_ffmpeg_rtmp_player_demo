#include <QGLPlayerWidget.h>

QGLPlayerWidget::QGLPlayerWidget(QWidget *parent)
    : QOpenGLWidget(parent),
      m_audioPlayer(new QAudioPlayer(8192 * 16))
{
}
void QGLPlayerWidget::start_preview(const std::string &media_url)
{
    FFmpegPlayer::start_preview(media_url);
    m_audioPlayer->start_consume_audio();
}
void QGLPlayerWidget::stop_preview()
{
    FFmpegPlayer::stop_preview();
    m_audioPlayer->stop_consume_audio();
}
void QGLPlayerWidget::initializeGL()
{
    initializeOpenGLFunctions();

    // 设置OpenGL状态，例如清除颜色和启用纹理2D
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_TEXTURE_2D);
}
void QGLPlayerWidget::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
}
void QGLPlayerWidget::paintGL()
{
    qDebug() << __FUNCTION__;
    if(!FFmpegPlayer::frame_consumed.load(std::memory_order_acquire))
    {
        if(init_texture_flag.load(std::memory_order_acquire))
        {
            // 创建纹理对象
            glGenTextures(1, &textureID);
            glBindTexture(GL_TEXTURE_2D, textureID);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_frame_cache->m_cache->width, m_frame_cache->m_cache->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            init_texture_flag.store(false, std::memory_order_release);
        }
        glClear(GL_COLOR_BUFFER_BIT);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                        m_frame_cache->m_cache->width,
                        m_frame_cache->m_cache->height, GL_RGBA, GL_UNSIGNED_BYTE, m_frame_cache->m_cache->data[0]);

        // 绘制纹理
        glBegin(GL_QUADS);
        glTexCoord2f(0.0, 1.0); glVertex2f(-1.0, -1.0);  // 左下角
        glTexCoord2f(1.0, 1.0); glVertex2f(1.0, -1.0);   // 右下角
        glTexCoord2f(1.0, 0.0); glVertex2f(1.0, 1.0);    // 右上角
        glTexCoord2f(0.0, 0.0); glVertex2f(-1.0, 1.0);   // 左上角
        glEnd();
        FFmpegPlayer::frame_consumed.store(true, std::memory_order_release);
    }
}

void QGLPlayerWidget::on_new_frame_avaliable()
{
    this->update();
}
void QGLPlayerWidget::on_start_preview([[maybe_unused]]const std::string& media_url)
{
    init_texture_flag.store(true, std::memory_order_release);    
}

void QGLPlayerWidget::on_stop_preview([[maybe_unused]]const std::string& media_url)
{
    init_texture_flag.store(false, std::memory_order_release);
}

void QGLPlayerWidget::on_new_audio_frame_avaliable(std::shared_ptr<FrameCache> m_frame_cache)
{
    if(m_audioPlayer)
    {
        m_audioPlayer->write(reinterpret_cast<const char*>(m_frame_cache->m_cache->data[0]), m_frame_cache->m_cache->linesize[0]);
    }
//    qDebug() << __FUNCTION__ << bytes_written << m_frame_cache->m_cache->linesize[0];
}
