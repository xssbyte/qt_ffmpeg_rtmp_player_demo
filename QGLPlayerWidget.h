#ifndef QGLPLAYERWIDGET_H
#define QGLPLAYERWIDGET_H

#include <FFmpegPlayer.h>

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <functional>
#include <QImage>

class QGLPlayerWidget : public QOpenGLWidget, protected QOpenGLFunctions, public FFmpegPlayer
{
    Q_OBJECT

public:
    QGLPlayerWidget(QWidget *parent = nullptr)
        : QOpenGLWidget(parent)
    {
        image = QImage(640, 480, QImage::Format_RGBA8888);

        image.fill(Qt::blue);

        videoFrameData = image.bits();
    }
public slots:
    void start_preview(const std::string &media_url)
    {
        FFmpegPlayer::start_preview(media_url);
    }
    void stop_preview()
    {
        FFmpegPlayer::stop_preview();
    }

    void initializeGL() override
    {
        initializeOpenGLFunctions();

        // 设置OpenGL状态，例如清除颜色和启用纹理2D
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glEnable(GL_TEXTURE_2D);

        // 创建纹理对象
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    }
    void resizeGL(int w, int h) override
    {
        glViewport(0, 0, w, h);
    }
    void paintGL() override
    {
        qDebug() << "paintGL";

        // 渲染帧数据到纹理
        glClear(GL_COLOR_BUFFER_BIT);

        // 使用互斥锁保护纹理数据
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, videoFrameData);

        // 绘制纹理
        glBegin(GL_QUADS);
        glTexCoord2f(0.0, 0.0); glVertex2f(-1.0, -1.0);
        glTexCoord2f(1.0, 0.0); glVertex2f(1.0, -1.0);
        glTexCoord2f(1.0, 1.0); glVertex2f(1.0, 1.0);
        glTexCoord2f(0.0, 1.0); glVertex2f(-1.0, 1.0);
        glEnd();
    }

public slots:
protected:
    void on_new_frame_avaliable(uint8_t* data,int w,int h) override
    {
//        width = w;
//        height = h;
        width = 640;
        height = 480;
///       videoFrameData = data;
        QMetaObject::invokeMethod(this, "update", Qt::BlockingQueuedConnection);
        qDebug() << "on_new_frame_avaliable";
    }
    void on_start_preview(const std::string& url) override
    {

    }
    void on_stop_preview(const std::string& url) override
    {

    }
    void on_ffmpeg_error() override
    {

    }

private:
    GLuint textureID;
    int width = 640;
    int height = 480;
    uint8_t* videoFrameData = nullptr;
    QImage image;
};

#endif // QGLPLAYERWIDGET_H
