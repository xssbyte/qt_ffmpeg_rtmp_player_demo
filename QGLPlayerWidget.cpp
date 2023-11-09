#include <QGLPlayerWidget.h>
#include <QDateTime> // for log
#include <QStyle>

QGLPlayerWidget::QGLPlayerWidget(QWidget *parent)
    : QOpenGLWidget(parent),
      m_audioPlayer(new QAudioPlayer(8192 * 16))
{
    setAttribute(Qt::WA_StaticContents);
}
QGLPlayerWidget::~QGLPlayerWidget()
{
}
int QGLPlayerWidget::start_preview(const std::string &media_url)
{
    m_audioPlayer->start_consume_audio();
    return FFmpegPlayer::start_preview(media_url);
}
int QGLPlayerWidget::stop_preview()
{
    m_audioPlayer->stop_consume_audio();
    return FFmpegPlayer::stop_preview();
}

void QGLPlayerWidget::initializeGL()
{
    initializeOpenGLFunctions();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_TEXTURE_2D);
}
void QGLPlayerWidget::setup_viewport(int w, int h)
{
    if(!w||!h)
        return;
    float current_aspectRatio = float(w) / float(h);
    // 计算需要填充黑色背景的区域
    int x, y, newWidth, newHeight;

    if (current_aspectRatio > aspectRatio) {
        // 视口宽高比更宽，添加黑边在上下
        newWidth = h * aspectRatio;
        newHeight = h;
        x = (w - newWidth) / 2;
        y = 0;
    } else {
        // 视口宽高比更窄，添加黑边在两侧
        newWidth = w;
        newHeight = w / aspectRatio;
        x = 0;
        y = (h - newHeight) / 2;
    }
    viewport_w = newWidth;
    viewport_h = newHeight;
    viewport_x = x;
    viewport_y = y;
}
void QGLPlayerWidget::resizeGL(int w, int h)
{
    qDebug() << __FUNCTION__ << "resize->" << w << "x" << h;
    //需要保证播放宽高比使用
    setup_viewport(w, h);
    glViewport(viewport_x, viewport_y, viewport_w, viewport_h);
    //需要拉伸使用
//    glViewport(0, 0, w, h);
}
void QGLPlayerWidget::paintGL()
{
//    qDebug() << __FUNCTION__ << QDateTime::currentDateTime().toMSecsSinceEpoch();
    if(!FFmpegPlayer::frame_consumed.load(std::memory_order_acquire))
    {
        glViewport(viewport_x, viewport_y, viewport_w, viewport_h);
        glClear(GL_COLOR_BUFFER_BIT);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                        m_frame_cache->m_cache->width,
                        m_frame_cache->m_cache->height, GL_RGBA, GL_UNSIGNED_BYTE, m_frame_cache->m_cache->data[0]);
        glBegin(GL_QUADS);
        glTexCoord2f(0.0, 1.0); glVertex2f(-1.0, -1.0);  // 左下角
        glTexCoord2f(1.0, 1.0); glVertex2f(1.0, -1.0);   // 右下角
        glTexCoord2f(1.0, 0.0); glVertex2f(1.0, 1.0);    // 右上角
        glTexCoord2f(0.0, 0.0); glVertex2f(-1.0, 1.0);   // 左上角
        glEnd();
        FFmpegPlayer::frame_consumed.store(true, std::memory_order_release);
    }
}

void QGLPlayerWidget::on_preview_start(const std::string& media_url, const int width, const int height)
{
    qDebug() << __FUNCTION__  << width << height;
    aspectRatio = (float)width / (float)height;
    setup_viewport(QWidget::width(), QWidget::height());
    QMetaObject::invokeMethod(this, [&](){
        makeCurrent();
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }, Qt::BlockingQueuedConnection);
    emit sig_on_preview_start(QString(media_url.c_str()), width, height);
}
void QGLPlayerWidget::on_preview_stop(const std::string& media_url)
{
    qDebug() << __FUNCTION__ ;
    aspectRatio = 0;
    emit sig_on_preview_stop(QString(media_url.c_str()));
}
void QGLPlayerWidget::on_recorder_start(const std::string& file)
{
    qDebug() << __FUNCTION__ ;
    emit sig_on_record_start(QString(file.c_str()));
}
void QGLPlayerWidget::on_recorder_stop(const std::string& file)
{
    qDebug() << __FUNCTION__ ;
    emit sig_on_record_stop(QString(file.c_str()));
}

void QGLPlayerWidget::on_new_frame_avaliable(std::shared_ptr<FrameCache> m_frame_cache)
{
    QMetaObject::invokeMethod(this, [&](){
        this->update();
    }, Qt::QueuedConnection);
}
void QGLPlayerWidget::on_new_audio_frame_avaliable(std::shared_ptr<FrameCache> m_frame_cache)
{
    if(m_audioPlayer)
    {
        m_audioPlayer->write(reinterpret_cast<const char*>(m_frame_cache->m_cache->data[0]), m_frame_cache->m_cache->linesize[0]);
    }
}
