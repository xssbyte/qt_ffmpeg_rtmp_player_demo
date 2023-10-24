#include <QGLPlayerWidget.h>
#include <QDateTime> // for log
#include <QStyle>

QGLPlayerWidget::QGLPlayerWidget(QWidget *parent)
    : QOpenGLWidget(parent),
      m_audioPlayer(new QAudioPlayer(8192 * 16))
{
}
QGLPlayerWidget::~QGLPlayerWidget()
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
void QGLPlayerWidget::start_local_record(const std::string &output_file)
{
    FFmpegPlayer::start_local_record(output_file);
}
void QGLPlayerWidget::stop_local_record()
{
    FFmpegPlayer::stop_local_record();
}
void QGLPlayerWidget::initializeGL()
{
    initializeOpenGLFunctions();
    // 创建并编译顶点着色器和片段着色器
    const char *vertexShaderSource = "#version 330 core\n"
                                     "layout(location = 0) in vec2 position;\n"
                                     "out vec2 texCoord;\n"
                                     "void main()\n"
                                     "{\n"
                                     "    gl_Position = vec4(position, 0.0, 1.0);\n"
                                     "    texCoord = vec2((position.x + 1.0) / 2.0, 1.0 - (position.y + 1.0) / 2.0);\n"
                                     "}\n";

    const char *fragmentShaderSource = "#version 330 core\n"
                                       "in vec2 texCoord;\n"
                                       "out vec4 FragColor;\n"
                                       "uniform sampler2D textureSampler;\n"
                                       "void main()\n"
                                       "{\n"
                                       "    FragColor = texture(textureSampler, texCoord);\n"
                                       "}\n";

    GLuint vertexShader, fragmentShader;
    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    // 创建着色器程序
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // 删除着色器对象，因为它们已经链接到着色器程序中
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // 创建矩形的顶点数据
    float vertices[] = {
        -1.0f, -1.0f,
        1.0f, -1.0f,
        1.0f,  1.0f,
        -1.0f,  1.0f
    };

    // 创建顶点缓冲对象并传递数据
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // 设置顶点属性指针
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

    // 设置OpenGL状态，例如清除颜色和启用纹理2D
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_TEXTURE_2D);
}
void QGLPlayerWidget::resizeGL(int w, int h)
{
    qDebug() << __FUNCTION__ << "resize->" << w << "x" << h;
    glViewport(0, 0, w, h);
}
void QGLPlayerWidget::paintGL()
{
    qDebug() << __FUNCTION__ << QDateTime::currentDateTime().toMSecsSinceEpoch();
    if(!FFmpegPlayer::frame_consumed.load(std::memory_order_acquire))
    {
        glClear(GL_COLOR_BUFFER_BIT);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                        m_frame_cache->m_cache->width,
                        m_frame_cache->m_cache->height, GL_RGBA, GL_UNSIGNED_BYTE, m_frame_cache->m_cache->data[0]);
//        // 绘制纹理
//        glBegin(GL_QUADS);
//        glTexCoord2f(0.0, 1.0); glVertex2f(-1.0, -1.0);  // 左下角
//        glTexCoord2f(1.0, 1.0); glVertex2f(1.0, -1.0);   // 右下角
//        glTexCoord2f(1.0, 0.0); glVertex2f(1.0, 1.0);    // 右上角
//        glTexCoord2f(0.0, 0.0); glVertex2f(-1.0, 1.0);   // 左上角
//        glEnd();

        // 使用现代OpenGL着色器
        glUseProgram(shaderProgram);

        // 绑定顶点缓冲对象
        glBindBuffer(GL_ARRAY_BUFFER, vbo);

        // 启用顶点属性
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

        // 绘制矩形
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

        // 禁用顶点属性
        glDisableVertexAttribArray(0);

        glUseProgram(0);

        FFmpegPlayer::frame_consumed.store(true, std::memory_order_release);
    }
}

void QGLPlayerWidget::on_new_frame_avaliable()
{
    QMetaObject::invokeMethod(this, [&](){
        this->update();
    }, Qt::QueuedConnection);
}
void QGLPlayerWidget::on_preview_start(const std::string& media_url, const int width, const int height)
{
    qDebug() << __FUNCTION__  << width << height;
    QMetaObject::invokeMethod(this, [&](){
        makeCurrent();
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }, Qt::QueuedConnection);
}

void QGLPlayerWidget::on_preview_stop(const std::string& media_url)
{
    qDebug() << __FUNCTION__ ;
}

void QGLPlayerWidget::on_new_audio_frame_avaliable(std::shared_ptr<FrameCache> m_frame_cache)
{
    if(m_audioPlayer)
    {
        m_audioPlayer->write(reinterpret_cast<const char*>(m_frame_cache->m_cache->data[0]), m_frame_cache->m_cache->linesize[0]);
    }
//    qDebug() << __FUNCTION__ << bytes_written << m_frame_cache->m_cache->linesize[0];
}
