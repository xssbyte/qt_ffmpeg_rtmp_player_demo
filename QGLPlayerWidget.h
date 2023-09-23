#ifndef QGLPLAYERWIDGET_H
#define QGLPLAYERWIDGET_H

#include <functional>

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QImage>
#include <QTimer>

#include <FFmpegPlayer.h>
#include <QAudioPlayer.h>
/**
* @brief         QGLPlayerWidget class
* 继承了QOpenGLWidget，同步绘制到opengl缓冲区，没有额外的memcpy
* @author        Samuel<13321133339@163.com>
* @date          2023/09/23
*/

class QGLPlayerWidget : public QOpenGLWidget, protected QOpenGLFunctions, protected FFmpegPlayer
{
    Q_OBJECT

public:
    QGLPlayerWidget(QWidget *parent = nullptr);
    ~QGLPlayerWidget() = default;
public slots:
    void start_preview(const std::string &media_url);
    void stop_preview();

    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

protected:
    void on_start_preview(const std::string& media_url) override;
    void on_stop_preview(const std::string& media_url) override;

    void on_new_frame_avaliable() override;
    void on_new_audio_frame_avaliable(std::shared_ptr<FrameCache> m_frame_cache) override;
private:
    GLuint textureID;
    int width;
    int height;
    std::atomic_bool init_texture_flag = true;
    std::unique_ptr<QAudioPlayer> m_audioPlayer;
};

#endif // QGLPLAYERWIDGET_H
