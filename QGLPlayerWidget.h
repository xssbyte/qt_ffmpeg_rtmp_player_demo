#ifndef QGLPLAYERWIDGET_H
#define QGLPLAYERWIDGET_H

#include <FFmpegPlayer.h>

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <functional>
#include <QImage>

/**
 * @brief The QGLPlayerWidget class
 * 继承了QOpenGLWidget，实现同步绘制，没有额外的memcpy
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
    void on_new_frame_avaliable() override;
private:
    GLuint textureID;
    int width = 640;
    int height = 360;
    QImage image;
};

#endif // QGLPLAYERWIDGET_H
