#ifndef QGLPLAYERWIDGET_H
#define QGLPLAYERWIDGET_H

#include <FFmpegPlayer.h>

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <functional>

class QGLPlayerWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    QGLPlayerWidget(QWidget *parent = nullptr)
        : QOpenGLWidget(parent),
          p_ffmpeg_player(new FFmpegPlayer())
    {
    }
public slots:
    void start_preview(const std::string &media_url)
    {
        p_ffmpeg_player->start_preview(media_url);
    }
    void stop_preview()
    {
        p_ffmpeg_player->stop_preview();
    }
    void initializeGL() override
    {
        initializeOpenGLFunctions();

        // 创建和编译OpenGL着色器程序
        shaderProgram = new QOpenGLShaderProgram;
        shaderProgram->addShaderFromSourceFile(QOpenGLShader::Vertex, "vertex_shader.glsl");
        shaderProgram->addShaderFromSourceFile(QOpenGLShader::Fragment, "fragment_shader.glsl");
        shaderProgram->link();
        shaderProgram->bind();

        // 设置顶点数据
        GLfloat vertices[] = {
            // 顶点坐标
            -1.0f, -1.0f,
            1.0f, -1.0f,
            -1.0f, 1.0f,
            1.0f, 1.0f,
        };

        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        // 设置顶点属性指针
        GLint posAttr = shaderProgram->attributeLocation("position");
        glVertexAttribPointer(posAttr, 2, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(posAttr);

        p_ffmpeg_player->set_preview_callback([&](uint8_t *data,int width,int height){
//            QMetaObject::invokeMethod(this, "updateFrame", Qt::QueuedConnection, Q_ARG(uint8_t*,data),Q_ARG(int, width),Q_ARG(int, 42));
            this->updateFrame(data,width,height);
        });
    }

    void paintGL() override
    {
        // 清除屏幕
        glClear(GL_COLOR_BUFFER_BIT);

        // 使用OpenGL着色器程序绘制
        shaderProgram->bind();
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        shaderProgram->release();
    }

public slots:
    void updateFrame(uint8_t *data, int w, int h)
    {
        makeCurrent();

        // 更新OpenGL纹理，假设data是RGB数据
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

        // 请求重新绘制
        update();
    }
protected:
//    void paintEvent(QPaintEvent *event) override
//    {

//    }
//    void resizeEvent(QResizeEvent *event) override
//    {

//    }
private:
    QOpenGLShaderProgram *shaderProgram;
    GLuint vbo;
    GLuint texture;

    std::function<void (uint8_t*/*data*/,int/*w*/,int/*h*/)> preview_callback;
    std::unique_ptr<FFmpegPlayer> p_ffmpeg_player;
};

#endif // QGLPLAYERWIDGET_H
