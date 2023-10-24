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
    ~QGLPlayerWidget();
public slots:
    void start_preview(const std::string &media_url);
    void stop_preview();
    void start_local_record(const std::string &output_file);
    void stop_local_record();

    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

protected:
    void on_preview_start(const std::string& media_url, const int width, const int height) override;
    void on_preview_stop(const std::string& media_url) override;
    void on_new_frame_avaliable() override;
    void on_new_audio_frame_avaliable(std::shared_ptr<FrameCache> m_frame_cache) override;
private:
    GLuint shaderProgram;  // 着色器程序
    GLuint vbo;  // 顶点缓冲对象
    GLuint textureID;
    int width;
    int height;
    std::unique_ptr<QAudioPlayer> m_audioPlayer;
};

#endif // QGLPLAYERWIDGET_H
