#include <QGLPlayerWidget.h>

QGLPlayerWidget::QGLPlayerWidget(QWidget *parent)
    : QOpenGLWidget(parent)
{
}
void QGLPlayerWidget::start_preview(const std::string &media_url)
{
    FFmpegPlayer::start_preview(media_url);
}
void QGLPlayerWidget::stop_preview()
{
    FFmpegPlayer::stop_preview();
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
//    glClear(GL_COLOR_BUFFER_BIT);
    if(!FFmpegPlayer::frame_consumed.load(std::memory_order_acquire))
    {
        if(textureID == 0)
        {
            // 创建纹理对象
            glGenTextures(1, &textureID);
            glBindTexture(GL_TEXTURE_2D, textureID);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
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