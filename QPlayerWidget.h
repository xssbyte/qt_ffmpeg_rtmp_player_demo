#ifndef QPLAYERWIDGET_H
#define QPLAYERWIDGET_H

#include <iostream>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <QDebug>
#include <QThread>
#include <QPainter>
#include <QWidget>
#include <FFmpegPlayer.h>

/**
 * @brief The QPlayerWidget class
 * 继承QWidget，使用qt框架的UI更新事件。先转为QImage，然后通过调用update()实现刷新上屏
 * 这种做法实现最简单，对事件循环压力比较大，每一帧都需要同步更新UI，帧率等于视频帧率
 * 不适合预览，但是适合做取帧和抽帧
 * 对于预览，最好的方案是使用ffmpeg+opengl主动控制上屏，这种做法不依赖于qt的事件循环
 */

class QPlayerWidget : public QWidget
{
    Q_OBJECT
public:
    QPlayerWidget(QWidget *parent = nullptr);
public slots:
    void start_preview(const std::string &media_url);
    void stop_preview();
    void set_preview_callback(std::function<void (uint8_t*/*data*/,int/*w*/,int/*h*/)> callback);
protected:
    void paintEvent(QPaintEvent *event);
private:
    std::function<void (uint8_t*/*data*/,int/*w*/,int/*h*/)> preview_callback;
    std::unique_ptr<FFmpegPlayer> p_ffmpeg_player;
    std::atomic_bool frame_avaliable = false;
    QImage m_image;
};

#endif // QPLAYERWIDGET_H
