#ifndef QGLPLAYERWIDGET_H
#define QGLPLAYERWIDGET_H

#include <functional>

#include <QtWidgets>
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
* 继承了QOpenGLWidget，同步绘制到opengl缓冲区，没有memcpy
* @author        Samuel<13321133339@163.com>
* @date          2023/09/23
*/

class QGLPlayerWidget : public QOpenGLWidget, protected QOpenGLFunctions, public FFmpegPlayer
{
    Q_OBJECT

public:
    QGLPlayerWidget(QWidget *parent = nullptr);
    ~QGLPlayerWidget();
signals:
    void sig_on_preview_start(const QString &media_url, const int width, const int heigh);
    void sig_on_preview_stop(const QString &media_url);
    void sig_on_record_start(const QString& file);
    void sig_on_record_stop(const QString& file);

public slots:
    int start_preview(const std::string &media_url);
    int stop_preview();

    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

protected:
    void on_preview_start(const std::string& media_url, const int width, const int height) override;
    void on_preview_stop(const std::string& media_url) override;
    void on_recorder_start(const std::string& file) override;
    void on_recorder_stop(const std::string& file) override;

    void on_new_frame_avaliable(std::shared_ptr<FrameCache> m_frame_cache) override;
    void on_new_audio_frame_avaliable(std::shared_ptr<FrameCache> m_frame_cache) override;
private:

    void setup_viewport(int view_w, int view_h);
    GLuint textureID;
    int viewport_x = 0, viewport_y = 0, viewport_w = 0, viewport_h = 0;
    float aspectRatio = 0;
    std::mutex render_mutex;
    std::unique_ptr<QAudioPlayer> m_audioPlayer;
};

#endif // QGLPLAYERWIDGET_H
